
:mod:`calendar` --- General calendar-related functions
======================================================

.. module:: calendar
   :synopsis: Functions for working with calendars, including some emulation of the Unix cal
              program.
.. sectionauthor:: Drew Csillag <drew_csillag@geocities.com>


This module allows you to output calendars like the Unix :program:`cal` program,
and provides additional useful functions related to the calendar. By default,
these calendars have Monday as the first day of the week, and Sunday as the last
(the European convention). Use :func:`setfirstweekday` to set the first day of
the week to Sunday (6) or to any other weekday.  Parameters that specify dates
are given as integers. For related
functionality, see also the :mod:`datetime` and :mod:`time` modules.

Most of these functions and classses rely on the :mod:`datetime` module which
uses an idealized calendar, the current Gregorian calendar indefinitely extended
in both directions.  This matches the definition of the "proleptic Gregorian"
calendar in Dershowitz and Reingold's book "Calendrical Calculations", where
it's the base calendar for all computations.


.. class:: Calendar([firstweekday])

   Creates a :class:`Calendar` object. *firstweekday* is an integer specifying the
   first day of the week. ``0`` is Monday (the default), ``6`` is Sunday.

   A :class:`Calendar` object provides several methods that can be used for
   preparing the calendar data for formatting. This class doesn't do any formatting
   itself. This is the job of subclasses.

   .. versionadded:: 2.5

:class:`Calendar` instances have the following methods:


.. method:: Calendar.iterweekdays(weekday)

   Return an iterator for the week day numbers that will be used for one week.
   The first value from the iterator will be the same as the value of the
   :attr:`firstweekday` property.


.. method:: Calendar.itermonthdates(year, month)

   Return an iterator for the month *month* (1-12) in the year *year*. This
   iterator will return all days (as :class:`datetime.date` objects) for the month
   and all days before the start of the month or after the end of the month that
   are required to get a complete week.


.. method:: Calendar.itermonthdays2(year, month)

   Return an iterator for the month *month* in the year *year* similar to
   :meth:`itermonthdates`. Days returned will be tuples consisting of a day number
   and a week day number.


.. method:: Calendar.itermonthdays(year, month)

   Return an iterator for the month *month* in the year *year* similar to
   :meth:`itermonthdates`. Days returned will simply be day numbers.


.. method:: Calendar.monthdatescalendar(year, month)

   Return a list of the weeks in the month *month* of the *year* as full weeks.
   Weeks are lists of seven :class:`datetime.date` objects.


.. method:: Calendar.monthdays2calendar(year, month)

   Return a list of the weeks in the month *month* of the *year* as full weeks.
   Weeks are lists of seven tuples of day numbers and weekday numbers.


.. method:: Calendar.monthdayscalendar(year, month)

   Return a list of the weeks in the month *month* of the *year* as full weeks.
   Weeks are lists of seven day numbers.


.. method:: Calendar.yeardatescalendar(year[, width])

   Return the data for the specified year ready for formatting. The return value
   is a list of month rows. Each month row contains up to *width* months
   (defaulting to 3). Each month contains between 4 and 6 weeks and each week
   contains 1--7 days. Days are :class:`datetime.date` objects.


.. method:: Calendar.yeardays2calendar(year[, width])

   Return the data for the specified year ready for formatting (similar to
   :meth:`yeardatescalendar`). Entries in the week lists are tuples of day
   numbers and weekday numbers. Day numbers outside this month are zero.


.. method:: Calendar.yeardayscalendar(year[, width])

   Return the data for the specified year ready for formatting (similar to
   :meth:`yeardatescalendar`). Entries in the week lists are day numbers. Day
   numbers outside this month are zero.


.. class:: TextCalendar([firstweekday])

   This class can be used to generate plain text calendars.

   .. versionadded:: 2.5

:class:`TextCalendar` instances have the following methods:


.. method:: TextCalendar.formatmonth(theyear, themonth[, w[, l]])

   Return a month's calendar in a multi-line string. If *w* is provided, it
   specifies the width of the date columns, which are centered. If *l* is given,
   it specifies the number of lines that each week will use. Depends on the
   first weekday as specified in the constructor or set by the
   :meth:`setfirstweekday` method.


.. method:: TextCalendar.prmonth(theyear, themonth[, w[, l]])

   Print a month's calendar as returned by :meth:`formatmonth`.


.. method:: TextCalendar.formatyear(theyear, themonth[, w[, l[, c[, m]]]])

   Return a *m*-column calendar for an entire year as a multi-line string.
   Optional parameters *w*, *l*, and *c* are for date column width, lines per
   week, and number of spaces between month columns, respectively. Depends on
   the first weekday as specified in the constructor or set by the
   :meth:`setfirstweekday` method.  The earliest year for which a calendar can
   be generated is platform-dependent.


.. method:: TextCalendar.pryear(theyear[, w[, l[, c[, m]]]])

   Print the calendar for an entire year as returned by :meth:`formatyear`.


.. class:: HTMLCalendar([firstweekday])

   This class can be used to generate HTML calendars.

   .. versionadded:: 2.5

:class:`HTMLCalendar` instances have the following methods:


.. method:: HTMLCalendar.formatmonth(theyear, themonth[, withyear])

   Return a month's calendar as an HTML table. If *withyear* is true the year will
   be included in the header, otherwise just the month name will be used.


.. method:: HTMLCalendar.formatyear(theyear, themonth[, width])

   Return a year's calendar as an HTML table. *width* (defaulting to 3) specifies
   the number of months per row.


.. method:: HTMLCalendar.formatyearpage(theyear[, width[, css[, encoding]]])

   Return a year's calendar as a complete HTML page. *width* (defaulting to 3)
   specifies the number of months per row. *css* is the name for the cascading
   style sheet to be used. :const:`None` can be passed if no style sheet should be
   used. *encoding* specifies the encoding to be used for the output (defaulting to
   the system default encoding).


.. class:: LocaleTextCalendar([firstweekday[, locale]])

   This subclass of :class:`TextCalendar` can be passed a locale name in the
   constructor and will return month and weekday names in the specified locale. If
   this locale includes an encoding all strings containing month and weekday names
   will be returned as unicode.

   .. versionadded:: 2.5


.. class:: LocaleHTMLCalendar([firstweekday[, locale]])

   This subclass of :class:`HTMLCalendar` can be passed a locale name in the
   constructor and will return month and weekday names in the specified locale. If
   this locale includes an encoding all strings containing month and weekday names
   will be returned as unicode.

   .. versionadded:: 2.5

For simple text calendars this module provides the following functions.


.. function:: setfirstweekday(weekday)

   Sets the weekday (``0`` is Monday, ``6`` is Sunday) to start each week. The
   values :const:`MONDAY`, :const:`TUESDAY`, :const:`WEDNESDAY`, :const:`THURSDAY`,
   :const:`FRIDAY`, :const:`SATURDAY`, and :const:`SUNDAY` are provided for
   convenience. For example, to set the first weekday to Sunday::

      import calendar
      calendar.setfirstweekday(calendar.SUNDAY)

   .. versionadded:: 2.0


.. function:: firstweekday()

   Returns the current setting for the weekday to start each week.

   .. versionadded:: 2.0


.. function:: isleap(year)

   Returns :const:`True` if *year* is a leap year, otherwise :const:`False`.


.. function:: leapdays(y1, y2)

   Returns the number of leap years in the range from *y1* to *y2* (exclusive),
   where *y1* and *y2* are years.

   .. versionchanged:: 2.0
      This function didn't work for ranges spanning a century change in Python
      1.5.2.


.. function:: weekday(year, month, day)

   Returns the day of the week (``0`` is Monday) for *year* (``1970``--...),
   *month* (``1``--``12``), *day* (``1``--``31``).


.. function:: weekheader(n)

   Return a header containing abbreviated weekday names. *n* specifies the width in
   characters for one weekday.


.. function:: monthrange(year, month)

   Returns weekday of first day of the month and number of days in month,  for the
   specified *year* and *month*.


.. function:: monthcalendar(year, month)

   Returns a matrix representing a month's calendar.  Each row represents a week;
   days outside of the month a represented by zeros. Each week begins with Monday
   unless set by :func:`setfirstweekday`.


.. function:: prmonth(theyear, themonth[, w[, l]])

   Prints a month's calendar as returned by :func:`month`.


.. function:: month(theyear, themonth[, w[, l]])

   Returns a month's calendar in a multi-line string using the :meth:`formatmonth`
   of the :class:`TextCalendar` class.

   .. versionadded:: 2.0


.. function:: prcal(year[, w[, l[c]]])

   Prints the calendar for an entire year as returned by  :func:`calendar`.


.. function:: calendar(year[, w[, l[c]]])

   Returns a 3-column calendar for an entire year as a multi-line string using the
   :meth:`formatyear` of the :class:`TextCalendar` class.

   .. versionadded:: 2.0


.. function:: timegm(tuple)

   An unrelated but handy function that takes a time tuple such as returned by the
   :func:`gmtime` function in the :mod:`time` module, and returns the corresponding
   Unix timestamp value, assuming an epoch of 1970, and the POSIX encoding.  In
   fact, :func:`time.gmtime` and :func:`timegm` are each others' inverse.

   .. versionadded:: 2.0

The :mod:`calendar` module exports the following data attributes:


.. data:: day_name

   An array that represents the days of the week in the current locale.


.. data:: day_abbr

   An array that represents the abbreviated days of the week in the current locale.


.. data:: month_name

   An array that represents the months of the year in the current locale.  This
   follows normal convention of January being month number 1, so it has a length of
   13 and  ``month_name[0]`` is the empty string.


.. data:: month_abbr

   An array that represents the abbreviated months of the year in the current
   locale.  This follows normal convention of January being month number 1, so it
   has a length of 13 and  ``month_abbr[0]`` is the empty string.


.. seealso::

   Module :mod:`datetime`
      Object-oriented interface to dates and times with similar functionality to the
      :mod:`time` module.

   Module :mod:`time`
      Low-level time related functions.

