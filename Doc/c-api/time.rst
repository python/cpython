.. highlight:: c

PyTime C API
============

.. versionadded:: 3.13

The clock C API provides access to system clocks and time conversion functions.
It is similar to the Python :mod:`time` module.

For C API related to the :mod:`datetime` module, see :ref:`datetimeobjects`.


Types
-----

.. c:type:: PyTime_t

   A timestamp or duration.

   The reference point for timestamps depends on the clock used. For example,
   :c:func:`PyTime_Time` returns timestamps relative to the UNIX epoch.

   ``PyTime_t`` is a signed integral type. It is possible to add and
   subtract such values directly.
   Keep in mind that C overflow rules do apply.

   ``PyTime_t`` can represent time with nanosecond or better resolution.
   The unit used is an internal implementation detail; use
   :c:func:`PyTime_AsNanoseconds` or :c:func:`PyTime_AsSecondsDouble`
   to convert to a given unit.

   The supported range is at least around [-292.3 years; +292.3 years].
   Using the Unix epoch (January 1st, 1970) as reference, the supported date
   range is at least around [1677-09-21; 2262-04-11].
   The exact limits are exposed as constants.

   .. c:var:: PyTime_t PyTime_MIN

      Minimum value of :c:type:`PyTime_t`.

   .. c:var:: PyTime_t PyTime_MAX

      Maximum value of :c:type:`PyTime_t`.


Clock Functions
---------------

The following functions take a pointer to a :c:expr:`PyTime_t` that they
set to the value of a particular clock.
Details of each clock are given in the documentation of the corresponding
Python function.

The functions return 0 on success, or -1 (with an exception set) on failure.

On integer overflow, they set the :c:data:`PyExc_OverflowError` exception and
set :c:expr:`*result` to the value clamped to the ``[PyTime_MIN; PyTime_MAX]``
range.
(On current systems, integer overflows are likely caused by misconfigured
system time.)

.. c:function:: int PyTime_Monotonic(PyTime_t *result)

   Read the monotonic clock.
   See :func:`time.monotonic` for important details on this clock.

.. c:function:: int PyTime_PerfCounter(PyTime_t *result)

   Read the performance counter.
   See :func:`time.perf_counter` for important details on this clock.

.. c:function:: int PyTime_Time(PyTime_t *result)

   Read the “wall clock” time.
   See :func:`time.time` for details important on this clock.


Conversion functions
--------------------

.. c:function:: double PyTime_AsSecondsDouble(PyTime_t t)

   Convert a timestamp to a number of seconds as a C :c:expr:`double`.

   The function cannot fail, but note that :c:expr:`double` has limited
   accuracy for large values.

.. c:function:: int PyTime_AsNanoseconds(PyTime_t t, int64_t *result)

   Convert a timestamp to a number of nanoseconds as a 64-bit signed integer.

   Returns 0 on success, or -1 (with an exception set) on failure.
