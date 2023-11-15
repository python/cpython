.. highlight:: c

PyTime C API
============

.. versionadded:: 3.13

PyTime API
----------

The PyTime_t API is written to use timestamp and timeout values stored in
various formats and to read clocks with a resolution of one nanosecond.

The :c:type:`PyTime_t` type is signed to support negative timestamps. The
supported range is around [-292.3 years; +292.3 years]. Using the Unix epoch
(January 1st, 1970) as reference, the supported date range is around
[1677-09-21; 2262-04-11].


Types
-----

.. c:type:: PyTime_t

   Timestamp type with subsecond precision: 64-bit signed integer.

   This type can be used to store a duration. Indirectly, it can be used to
   store a date relative to a reference date, such as the UNIX epoch.


Constants
---------

.. c:var:: PyTime_t PyTime_MIN

   Minimum value of the :c:type:`PyTime_t` type.

   :c:var:`PyTime_MIN` nanoseconds is around -292.3 years.

.. c:var:: PyTime_t PyTime_MAX

   Maximum value of the :c:type:`PyTime_t` type.

   :c:var:`PyTime_MAX` nanoseconds is around +292.3 years.


Functions
---------

.. c:function:: double PyTime_AsSecondsDouble(PyTime_t t)

   Convert a timestamp to a number of seconds as a C :c:expr:`double`.

   The function cannot fail.


.. c:function:: PyTime_t PyTime_Monotonic(void)

   Get the monotonic clock: clock that cannot go backwards.

   The monotonic clock is not affected by system clock updates. The reference
   point of the returned value is undefined, so that only the difference
   between the results of consecutive calls is valid.

   If reading the clock fails, silently ignore the error and return 0.

   On integer overflow, silently ignore the overflow and clamp the clock to
   the ``[PyTime_MIN; PyTime_MAX]`` range.

   See also the :func:`time.monotonic` function.


.. c:function:: PyTime_t PyTime_PerfCounter(void)

   Get the performance counter: clock with the highest available resolution to
   measure a short duration.

   The performance counter does include time elapsed during sleep and is
   system-wide. The reference point of the returned value is undefined, so that
   only the difference between the results of two calls is valid.

   If reading the clock fails, silently ignore the error and return 0.

   On integer overflow, silently ignore the overflow and clamp the clock to
   the ``[PyTime_MIN; PyTime_MAX]`` range.

   See also the :func:`time.perf_counter` function.


.. c:function:: PyTime_t PyTime_Time(void)

   Get the system clock.

   The system clock can be changed automatically (e.g. by a NTP daemon) or
   manually by the system administrator. So it can also go backward.  Use
   :c:func:`PyTime_Monotonic` to use a monotonic clock.

   If reading the clock fails, silently ignore the error and return ``0``.

   On integer overflow, silently ignore the overflow and clamp the clock to
   the ``[PyTime_MIN; PyTime_MAX]`` range.

   See also the :func:`time.time` function.
