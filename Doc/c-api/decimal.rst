.. sectionauthor:: Stefan Krah

.. highlight:: c


Decimal capsule API
===================

Capsule API functions can be used in the same manner as regular library
functions, provided that the API has been initialized.


Initialize
----------

Typically, a C extension module that uses the decimal API will do these
steps in its init function:

.. code-block::

    #include "pydecimal.h"

    static int decimal_initialized = 0;
    if (!decimal_initialized) {
        if (import_decimal() < 0) {
            return NULL;
        }

        decimal_initialized = 1;
    }


Type checking, predicates, accessors
------------------------------------

.. c:function:: int PyDec_TypeCheck(const PyObject *dec)

   Return 1 if ``dec`` is a Decimal, 0 otherwise.  This function does not set
   any exceptions.


.. c:function:: int PyDec_IsSpecial(const PyObject *dec)

   Return 1 if ``dec`` is ``NaN``, ``sNaN`` or ``Infinity``, 0 otherwise.

   Set TypeError and return -1 if ``dec`` is not a Decimal.  It is guaranteed that
   this is the only failure mode, so if ``dec`` has already been type-checked, no
   errors can occur and the function can be treated as a simple predicate.


.. c:function:: int PyDec_IsNaN(const PyObject *dec)

   Return 1 if ``dec`` is ``NaN`` or ``sNaN``, 0 otherwise.

   Set TypeError and return -1 if ``dec`` is not a Decimal.  It is guaranteed that
   this is the only failure mode, so if ``dec`` has already been type-checked, no
   errors can occur and the function can be treated as a simple predicate.


.. c:function:: int PyDec_IsInfinite(const PyObject *dec)

   Return 1 if ``dec`` is ``Infinity``, 0 otherwise.

   Set TypeError and return -1 if ``dec`` is not a Decimal.  It is guaranteed that
   this is the only failure mode, so if ``dec`` has already been type-checked, no
   errors can occur and the function can be treated as a simple predicate.


.. c:function:: int64_t PyDec_GetDigits(const PyObject *dec)

   Return the number of digits in the coefficient.  For ``Infinity``, the
   number of digits is always zero.  Typically, the same applies to ``NaN``
   and ``sNaN``, but both of these can have a payload that is equivalent to
   a coefficient.  Therefore, ``NaNs`` can have a nonzero return value.

   Set TypeError and return -1 if ``dec`` is not a Decimal.  It is guaranteed that
   this is the only failure mode, so if ``dec`` has already been type-checked, no
   errors can occur and the function can be treated as a simple accessor.


Exact conversions between decimals and primitive C types
--------------------------------------------------------

This API supports conversions for decimals with a coefficient up to 38 digits.

Data structures
~~~~~~~~~~~~~~~

The conversion functions use the following status codes and data structures:

.. code-block::

   /* status cases for getting a triple */
   enum mpd_triple_class {
     MPD_TRIPLE_NORMAL,
     MPD_TRIPLE_INF,
     MPD_TRIPLE_QNAN,
     MPD_TRIPLE_SNAN,
     MPD_TRIPLE_ERROR,
   };

   typedef struct {
     enum mpd_triple_class tag;
     uint8_t sign;
     uint64_t hi;
     uint64_t lo;
     int64_t exp;
   } mpd_uint128_triple_t;

The status cases are explained below.  ``sign`` is 0 for positive and 1 for negative.
``((uint128_t)hi << 64) + lo`` is the coefficient, ``exp`` is the exponent.

The data structure is called "triple" because the decimal triple (sign, coeff, exp)
is an established term and (``hi``, ``lo``) represents a single ``uint128_t`` coefficient.


Functions
~~~~~~~~~

.. c:function:: mpd_uint128_triple_t PyDec_AsUint128Triple(const PyObject *dec)

   Convert a decimal to a triple.  As above, it is guaranteed that the only
   Python failure mode is a TypeError, checks can be omitted if the type is
   known.

   For simplicity, the usage of the function and all special cases are
   explained in code form and comments:

.. code-block::

    triple = PyDec_AsUint128Triple(dec);
    switch (triple.tag) {
    case MPD_TRIPLE_QNAN:
        /*
         * Success: handle a quiet NaN.
         *   1) triple.sign is 0 or 1.
         *   2) triple.exp is always 0.
         *   3) If triple.hi or triple.lo are nonzero, the NaN has a payload.
         */
        break;

    case MPD_TRIPLE_SNAN:
        /*
         * Success: handle a signaling NaN.
         *   1) triple.sign is 0 or 1.
         *   2) triple.exp is always 0.
         *   3) If triple.hi or triple.lo are nonzero, the sNaN has a payload.
         */
        break;

    case MPD_TRIPLE_INF:
        /*
         * Success: handle Infinity.
         *   1) triple.sign is 0 or 1.
         *   2) triple.exp is always 0.
         *   3) triple.hi and triple.lo are always zero.
         */
        break;

    case MPD_TRIPLE_NORMAL:
        /* Success: handle a finite value. */
        break;

    case MPD_TRIPLE_ERROR:
        /* TypeError check: can be omitted if the type of dec is known. */
        if (PyErr_Occurred()) {
            return NULL;
        }

        /* Too large for conversion.  PyDec_AsUint128Triple() does not set an
           exception so applications can choose themselves.  Typically this
           would be a ValueError. */
        PyErr_SetString(PyExc_ValueError,
            "value out of bounds for a uint128 triple");
        return NULL;
    }

.. c:function:: PyObject *PyDec_FromUint128Triple(const mpd_uint128_triple_t *triple)

   Create a decimal from a triple.  The following rules must be observed for
   initializing the triple:

   1) ``triple.sign`` must always be 0 (for positive) or 1 (for negative).

   2) ``MPD_TRIPLE_QNAN``: ``triple.exp`` must be 0.  If ``triple.hi`` or ``triple.lo``
      are nonzero,  create a ``NaN`` with a payload.

   3) ``MPD_TRIPLE_SNAN``: ``triple.exp`` must be 0. If ``triple.hi`` or ``triple.lo``
      are nonzero,  create an ``sNaN`` with a payload.

   4) ``MPD_TRIPLE_INF``: ``triple.exp``, ``triple.hi`` and ``triple.lo`` must be zero.

   5) ``MPD_TRIPLE_NORMAL``: ``MPD_MIN_ETINY + 38 < triple.exp < MPD_MAX_EMAX - 38``.
      ``triple.hi`` and ``triple.lo`` can be chosen freely.

   6) ``MPD_TRIPLE_ERROR``: It is always an error to set this tag.


   If one of the above conditions is not met, the function returns ``NaN`` if
   the ``InvalidOperation`` trap is not set in the thread local context.  Otherwise,
   it sets the ``InvalidOperation`` exception and returns NULL.

   Additionally, though extremely unlikely give the small allocation sizes,
   the function can set ``MemoryError`` and return ``NULL``.


Advanced API
------------

This API enables the use of ``libmpdec`` functions.  Since Python is compiled with
hidden symbols, the API requires an external libmpdec and the ``mpdecimal.h``
header.


Functions
~~~~~~~~~

.. c:function:: PyObject *PyDec_Alloc(void)

   Return a new decimal that can be used in the ``result`` position of ``libmpdec``
   functions.

.. c:function:: mpd_t *PyDec_Get(PyObject *v)

   Get a pointer to the internal ``mpd_t`` of the decimal.  Decimals are immutable,
   so this function must only be used on a new Decimal that has been created by
   PyDec_Alloc().

.. c:function:: const mpd_t *PyDec_GetConst(const PyObject *v)

   Get a pointer to the constant internal ``mpd_t`` of the decimal.
