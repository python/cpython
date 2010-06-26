.. highlightlang:: c

.. _datetimeobjects:

DateTime Objects
----------------

Various date and time objects are supplied by the :mod:`datetime` module.
Before using any of these functions, the header file :file:`datetime.h` must be
included in your source (note that this is not included by :file:`Python.h`),
and the macro :cmacro:`PyDateTime_IMPORT` must be invoked, usually as part of
the module initialisation function.  The macro puts a pointer to a C structure
into a static variable, :cdata:`PyDateTimeAPI`, that is used by the following
macros.

Type-check macros:


.. cfunction:: int PyDate_Check(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_DateType` or a subtype of
   :cdata:`PyDateTime_DateType`.  *ob* must not be *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyDate_CheckExact(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_DateType`. *ob* must not be
   *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_Check(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_DateTimeType` or a subtype of
   :cdata:`PyDateTime_DateTimeType`.  *ob* must not be *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_CheckExact(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_DateTimeType`. *ob* must not
   be *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyTime_Check(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_TimeType` or a subtype of
   :cdata:`PyDateTime_TimeType`.  *ob* must not be *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyTime_CheckExact(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_TimeType`. *ob* must not be
   *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyDelta_Check(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_DeltaType` or a subtype of
   :cdata:`PyDateTime_DeltaType`.  *ob* must not be *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyDelta_CheckExact(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_DeltaType`. *ob* must not be
   *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyTZInfo_Check(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_TZInfoType` or a subtype of
   :cdata:`PyDateTime_TZInfoType`.  *ob* must not be *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyTZInfo_CheckExact(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_TZInfoType`. *ob* must not be
   *NULL*.

   .. versionadded:: 2.4

Macros to create objects:


.. cfunction:: PyObject* PyDate_FromDate(int year, int month, int day)

   Return a ``datetime.date`` object with the specified year, month and day.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyDateTime_FromDateAndTime(int year, int month, int day, int hour, int minute, int second, int usecond)

   Return a ``datetime.datetime`` object with the specified year, month, day, hour,
   minute, second and microsecond.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyTime_FromTime(int hour, int minute, int second, int usecond)

   Return a ``datetime.time`` object with the specified hour, minute, second and
   microsecond.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyDelta_FromDSU(int days, int seconds, int useconds)

   Return a ``datetime.timedelta`` object representing the given number of days,
   seconds and microseconds.  Normalization is performed so that the resulting
   number of microseconds and seconds lie in the ranges documented for
   ``datetime.timedelta`` objects.

   .. versionadded:: 2.4

Macros to extract fields from date objects.  The argument must be an instance of
:cdata:`PyDateTime_Date`, including subclasses (such as
:cdata:`PyDateTime_DateTime`).  The argument must not be *NULL*, and the type is
not checked:


.. cfunction:: int PyDateTime_GET_YEAR(PyDateTime_Date *o)

   Return the year, as a positive int.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_GET_MONTH(PyDateTime_Date *o)

   Return the month, as an int from 1 through 12.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_GET_DAY(PyDateTime_Date *o)

   Return the day, as an int from 1 through 31.

   .. versionadded:: 2.4

Macros to extract fields from datetime objects.  The argument must be an
instance of :cdata:`PyDateTime_DateTime`, including subclasses. The argument
must not be *NULL*, and the type is not checked:


.. cfunction:: int PyDateTime_DATE_GET_HOUR(PyDateTime_DateTime *o)

   Return the hour, as an int from 0 through 23.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_DATE_GET_MINUTE(PyDateTime_DateTime *o)

   Return the minute, as an int from 0 through 59.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_DATE_GET_SECOND(PyDateTime_DateTime *o)

   Return the second, as an int from 0 through 59.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_DATE_GET_MICROSECOND(PyDateTime_DateTime *o)

   Return the microsecond, as an int from 0 through 999999.

   .. versionadded:: 2.4

Macros to extract fields from time objects.  The argument must be an instance of
:cdata:`PyDateTime_Time`, including subclasses. The argument must not be *NULL*,
and the type is not checked:


.. cfunction:: int PyDateTime_TIME_GET_HOUR(PyDateTime_Time *o)

   Return the hour, as an int from 0 through 23.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_TIME_GET_MINUTE(PyDateTime_Time *o)

   Return the minute, as an int from 0 through 59.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_TIME_GET_SECOND(PyDateTime_Time *o)

   Return the second, as an int from 0 through 59.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_TIME_GET_MICROSECOND(PyDateTime_Time *o)

   Return the microsecond, as an int from 0 through 999999.

   .. versionadded:: 2.4

Macros for the convenience of modules implementing the DB API:


.. cfunction:: PyObject* PyDateTime_FromTimestamp(PyObject *args)

   Create and return a new ``datetime.datetime`` object given an argument tuple
   suitable for passing to ``datetime.datetime.fromtimestamp()``.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyDate_FromTimestamp(PyObject *args)

   Create and return a new ``datetime.date`` object given an argument tuple
   suitable for passing to ``datetime.date.fromtimestamp()``.

   .. versionadded:: 2.4
