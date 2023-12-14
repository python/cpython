.. highlight:: c

PyTime C API
============

.. versionadded:: 3.13

PyTime API
----------

The PyTime API provides access to system clocks and time conversion functions.
It is similar to the Python :mod:`time` module.


Types
-----

.. c:type:: PyTime_t

   A timestamp or duration in nanoseconds represented as a 64-bit signed
   integer.

   The reference point for timestamps depends on the clock used. For example,
   :c:func:`PyTime_Time` returns timestamps relative to the UNIX epoch.

   The supported range is around [-292.3 years; +292.3 years]. Using the Unix
   epoch (January 1st, 1970) as reference, the supported date range is around
   [1677-09-21; 2262-04-11].


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

   Return the value in nanoseconds of a monotonic clock, i.e. a clock
   that cannot go backwards. Similar to :func:`time.monotonic_ns`; see
   :func:`time.monotonic` for details.

   If reading the clock fails, silently ignore the error and return ``0``.

   On integer overflow, silently ignore the overflow and clamp the clock to
   the ``[PyTime_MIN; PyTime_MAX]`` range.


.. c:function:: PyTime_t PyTime_PerfCounter(void)

   Return the value in nanoseconds of a performance counter, i.e. a
   clock with the highest available resolution to measure a short duration.
   Similar to :func:`time.perf_counter_ns`; see :func:`time.perf_counter` for
   details.

   If reading the clock fails, silently ignore the error and return ``0``.

   On integer overflow, silently ignore the overflow and clamp the clock to
   the ``[PyTime_MIN; PyTime_MAX]`` range.


.. c:function:: PyTime_t PyTime_Time(void)

   Return the time in nanoseconds since the epoch_. Similar to
   :func:`time.time_ns`; see :func:`time.time` for details.

   If reading the clock fails, silently ignore the error and return ``0``.

   On integer overflow, silently ignore the overflow and clamp the clock to
   the ``[PyTime_MIN; PyTime_MAX]`` range.
