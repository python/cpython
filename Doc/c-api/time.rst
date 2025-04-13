.. highlight:: c

.. _c-api-time:

PyTime C API
============

.. versionadded:: 3.13

The clock C API provides access to system clocks.
It is similar to the Python :mod:`time` module.

For C API related to the :mod:`datetime` module, see :ref:`datetimeobjects`.


Types
-----

.. c:type:: PyTime_t

   A timestamp or duration in nanoseconds, represented as a signed 64-bit
   integer.

   The reference point for timestamps depends on the clock used. For example,
   :c:func:`PyTime_Time` returns timestamps relative to the UNIX epoch.

   The supported range is around [-292.3 years; +292.3 years].
   Using the Unix epoch (January 1st, 1970) as reference, the supported date
   range is around [1677-09-21; 2262-04-11].
   The exact limits are exposed as constants:

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

The functions return ``0`` on success, or ``-1`` (with an exception set)
on failure.

On integer overflow, they set the :c:data:`PyExc_OverflowError` exception and
set ``*result`` to the value clamped to the ``[PyTime_MIN; PyTime_MAX]``
range.
(On current systems, integer overflows are likely caused by misconfigured
system time.)

As any other C API (unless otherwise specified), the functions must be called
with an :term:`attached thread state`.

.. c:function:: int PyTime_Monotonic(PyTime_t *result)

   Read the monotonic clock.
   See :func:`time.monotonic` for important details on this clock.

.. c:function:: int PyTime_PerfCounter(PyTime_t *result)

   Read the performance counter.
   See :func:`time.perf_counter` for important details on this clock.

.. c:function:: int PyTime_Time(PyTime_t *result)

   Read the “wall clock” time.
   See :func:`time.time` for details important on this clock.


Raw Clock Functions
-------------------

Similar to clock functions, but don't set an exception on error and don't
require the caller to have an :term:`attached thread state`.

On success, the functions return ``0``.

On failure, they set ``*result`` to ``0`` and return ``-1``, *without* setting
an exception. To get the cause of the error, :term:`attach <attached thread state>` a :term:`thread state`,
and call the regular (non-``Raw``) function. Note that the regular function may succeed after
the ``Raw`` one failed.

.. c:function:: int PyTime_MonotonicRaw(PyTime_t *result)

   Similar to :c:func:`PyTime_Monotonic`,
   but don't set an exception on error and don't require an :term:`attached thread state`.

.. c:function:: int PyTime_PerfCounterRaw(PyTime_t *result)

   Similar to :c:func:`PyTime_PerfCounter`,
   but don't set an exception on error and don't require an :term:`attached thread state`.

.. c:function:: int PyTime_TimeRaw(PyTime_t *result)

   Similar to :c:func:`PyTime_Time`,
   but don't set an exception on error and don't require an :term:`attached thread state`.


Conversion functions
--------------------

.. c:function:: double PyTime_AsSecondsDouble(PyTime_t t)

   Convert a timestamp to a number of seconds as a C :c:expr:`double`.

   The function cannot fail, but note that :c:expr:`double` has limited
   accuracy for large values.
