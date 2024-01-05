.. highlight:: c

.. _datetimeobjects:

DateTime Objects
----------------

Various date and time objects are supplied by the :mod:`datetime` module.
Before using any of these functions, the header file :file:`datetime.h` must be
included in your source (note that this is not included by :file:`Python.h`),
and the macro :c:macro:`!PyDateTime_IMPORT` must be invoked, usually as part of
the module initialisation function.  The macro puts a pointer to a C structure
into a static variable, :c:data:`!PyDateTimeAPI`, that is used by the following
macros.

.. c:type:: PyDateTime_Date

   This subtype of :c:type:`PyObject` represents a Python date object.

.. c:type:: PyDateTime_DateTime

   This subtype of :c:type:`PyObject` represents a Python datetime object.

.. c:type:: PyDateTime_Time

   This subtype of :c:type:`PyObject` represents a Python time object.

.. c:type:: PyDateTime_Delta

   This subtype of :c:type:`PyObject` represents the difference between two datetime values.

.. c:var:: PyTypeObject PyDateTime_DateType

   This instance of :c:type:`PyTypeObject` represents the Python date type;
   it is the same object as :class:`datetime.date` in the Python layer.

.. c:var:: PyTypeObject PyDateTime_DateTimeType

   This instance of :c:type:`PyTypeObject` represents the Python datetime type;
   it is the same object as :class:`datetime.datetime` in the Python layer.

.. c:var:: PyTypeObject PyDateTime_TimeType

   This instance of :c:type:`PyTypeObject` represents the Python time type;
   it is the same object as :class:`datetime.time` in the Python layer.

.. c:var:: PyTypeObject PyDateTime_DeltaType

   This instance of :c:type:`PyTypeObject` represents Python type for
   the difference between two datetime values;
   it is the same object as :class:`datetime.timedelta` in the Python layer.

.. c:var:: PyTypeObject PyDateTime_TZInfoType

   This instance of :c:type:`PyTypeObject` represents the Python time zone info type;
   it is the same object as :class:`datetime.tzinfo` in the Python layer.


Macro for access to the UTC singleton:

.. c:var:: PyObject* PyDateTime_TimeZone_UTC

   Returns the time zone singleton representing UTC, the same object as
   :attr:`datetime.timezone.utc`.

   .. versionadded:: 3.7


Type-check macros:

.. c:function:: int PyDate_Check(PyObject *ob)

   Return true if *ob* is of type :c:data:`PyDateTime_DateType` or a subtype of
   :c:data:`!PyDateTime_DateType`.  *ob* must not be ``NULL``.  This function always
   succeeds.


.. c:function:: int PyDate_CheckExact(PyObject *ob)

   Return true if *ob* is of type :c:data:`PyDateTime_DateType`. *ob* must not be
   ``NULL``.  This function always succeeds.


.. c:function:: int PyDateTime_Check(PyObject *ob)

   Return true if *ob* is of type :c:data:`PyDateTime_DateTimeType` or a subtype of
   :c:data:`!PyDateTime_DateTimeType`.  *ob* must not be ``NULL``.  This function always
   succeeds.


.. c:function:: int PyDateTime_CheckExact(PyObject *ob)

   Return true if *ob* is of type :c:data:`PyDateTime_DateTimeType`. *ob* must not
   be ``NULL``.  This function always succeeds.


.. c:function:: int PyTime_Check(PyObject *ob)

   Return true if *ob* is of type :c:data:`PyDateTime_TimeType` or a subtype of
   :c:data:`!PyDateTime_TimeType`.  *ob* must not be ``NULL``.  This function always
   succeeds.


.. c:function:: int PyTime_CheckExact(PyObject *ob)

   Return true if *ob* is of type :c:data:`PyDateTime_TimeType`. *ob* must not be
   ``NULL``.  This function always succeeds.


.. c:function:: int PyDelta_Check(PyObject *ob)

   Return true if *ob* is of type :c:data:`PyDateTime_DeltaType` or a subtype of
   :c:data:`!PyDateTime_DeltaType`.  *ob* must not be ``NULL``.  This function always
   succeeds.


.. c:function:: int PyDelta_CheckExact(PyObject *ob)

   Return true if *ob* is of type :c:data:`PyDateTime_DeltaType`. *ob* must not be
   ``NULL``.  This function always succeeds.


.. c:function:: int PyTZInfo_Check(PyObject *ob)

   Return true if *ob* is of type :c:data:`PyDateTime_TZInfoType` or a subtype of
   :c:data:`!PyDateTime_TZInfoType`.  *ob* must not be ``NULL``.  This function always
   succeeds.


.. c:function:: int PyTZInfo_CheckExact(PyObject *ob)

   Return true if *ob* is of type :c:data:`PyDateTime_TZInfoType`. *ob* must not be
   ``NULL``.  This function always succeeds.


Macros to create objects:

.. c:function:: PyObject* PyDate_FromDate(int year, int month, int day)

   Return a :class:`datetime.date` object with the specified year, month and day.


.. c:function:: PyObject* PyDateTime_FromDateAndTime(int year, int month, int day, int hour, int minute, int second, int usecond)

   Return a :class:`datetime.datetime` object with the specified year, month, day, hour,
   minute, second and microsecond.


.. c:function:: PyObject* PyDateTime_FromDateAndTimeAndFold(int year, int month, int day, int hour, int minute, int second, int usecond, int fold)

   Return a :class:`datetime.datetime` object with the specified year, month, day, hour,
   minute, second, microsecond and fold.

   .. versionadded:: 3.6


.. c:function:: PyObject* PyTime_FromTime(int hour, int minute, int second, int usecond)

   Return a :class:`datetime.time` object with the specified hour, minute, second and
   microsecond.


.. c:function:: PyObject* PyTime_FromTimeAndFold(int hour, int minute, int second, int usecond, int fold)

   Return a :class:`datetime.time` object with the specified hour, minute, second,
   microsecond and fold.

   .. versionadded:: 3.6


.. c:function:: PyObject* PyDelta_FromDSU(int days, int seconds, int useconds)

   Return a :class:`datetime.timedelta` object representing the given number
   of days, seconds and microseconds.  Normalization is performed so that the
   resulting number of microseconds and seconds lie in the ranges documented for
   :class:`datetime.timedelta` objects.


.. c:function:: PyObject* PyTimeZone_FromOffset(PyObject *offset)

   Return a :class:`datetime.timezone` object with an unnamed fixed offset
   represented by the *offset* argument.

   .. versionadded:: 3.7


.. c:function:: PyObject* PyTimeZone_FromOffsetAndName(PyObject *offset, PyObject *name)

   Return a :class:`datetime.timezone` object with a fixed offset represented
   by the *offset* argument and with tzname *name*.

   .. versionadded:: 3.7


Macros to extract fields from date objects.  The argument must be an instance of
:c:type:`PyDateTime_Date`, including subclasses (such as
:c:type:`PyDateTime_DateTime`).  The argument must not be ``NULL``, and the type is
not checked:

.. c:function:: int PyDateTime_GET_YEAR(PyDateTime_Date *o)

   Return the year, as a positive int.


.. c:function:: int PyDateTime_GET_MONTH(PyDateTime_Date *o)

   Return the month, as an int from 1 through 12.


.. c:function:: int PyDateTime_GET_DAY(PyDateTime_Date *o)

   Return the day, as an int from 1 through 31.


Macros to extract fields from datetime objects.  The argument must be an
instance of :c:type:`PyDateTime_DateTime`, including subclasses. The argument
must not be ``NULL``, and the type is not checked:

.. c:function:: int PyDateTime_DATE_GET_HOUR(PyDateTime_DateTime *o)

   Return the hour, as an int from 0 through 23.


.. c:function:: int PyDateTime_DATE_GET_MINUTE(PyDateTime_DateTime *o)

   Return the minute, as an int from 0 through 59.


.. c:function:: int PyDateTime_DATE_GET_SECOND(PyDateTime_DateTime *o)

   Return the second, as an int from 0 through 59.


.. c:function:: int PyDateTime_DATE_GET_MICROSECOND(PyDateTime_DateTime *o)

   Return the microsecond, as an int from 0 through 999999.


.. c:function:: int PyDateTime_DATE_GET_FOLD(PyDateTime_DateTime *o)

   Return the fold, as an int from 0 through 1.

   .. versionadded:: 3.6


.. c:function:: PyObject* PyDateTime_DATE_GET_TZINFO(PyDateTime_DateTime *o)

   Return the tzinfo (which may be ``None``).

   .. versionadded:: 3.10


Macros to extract fields from time objects.  The argument must be an instance of
:c:type:`PyDateTime_Time`, including subclasses. The argument must not be ``NULL``,
and the type is not checked:

.. c:function:: int PyDateTime_TIME_GET_HOUR(PyDateTime_Time *o)

   Return the hour, as an int from 0 through 23.


.. c:function:: int PyDateTime_TIME_GET_MINUTE(PyDateTime_Time *o)

   Return the minute, as an int from 0 through 59.


.. c:function:: int PyDateTime_TIME_GET_SECOND(PyDateTime_Time *o)

   Return the second, as an int from 0 through 59.


.. c:function:: int PyDateTime_TIME_GET_MICROSECOND(PyDateTime_Time *o)

   Return the microsecond, as an int from 0 through 999999.


.. c:function:: int PyDateTime_TIME_GET_FOLD(PyDateTime_Time *o)

   Return the fold, as an int from 0 through 1.

   .. versionadded:: 3.6


.. c:function:: PyObject* PyDateTime_TIME_GET_TZINFO(PyDateTime_Time *o)

   Return the tzinfo (which may be ``None``).

   .. versionadded:: 3.10


Macros to extract fields from time delta objects.  The argument must be an
instance of :c:type:`PyDateTime_Delta`, including subclasses. The argument must
not be ``NULL``, and the type is not checked:

.. c:function:: int PyDateTime_DELTA_GET_DAYS(PyDateTime_Delta *o)

   Return the number of days, as an int from -999999999 to 999999999.

   .. versionadded:: 3.3


.. c:function:: int PyDateTime_DELTA_GET_SECONDS(PyDateTime_Delta *o)

   Return the number of seconds, as an int from 0 through 86399.

   .. versionadded:: 3.3


.. c:function:: int PyDateTime_DELTA_GET_MICROSECONDS(PyDateTime_Delta *o)

   Return the number of microseconds, as an int from 0 through 999999.

   .. versionadded:: 3.3


Macros for the convenience of modules implementing the DB API:

.. c:function:: PyObject* PyDateTime_FromTimestamp(PyObject *args)

   Create and return a new :class:`datetime.datetime` object given an argument
   tuple suitable for passing to :meth:`datetime.datetime.fromtimestamp()`.


.. c:function:: PyObject* PyDate_FromTimestamp(PyObject *args)

   Create and return a new :class:`datetime.date` object given an argument
   tuple suitable for passing to :meth:`datetime.date.fromtimestamp()`.
