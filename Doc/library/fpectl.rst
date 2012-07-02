
:mod:`fpectl` --- Floating point exception control
==================================================

.. module:: fpectl
   :platform: Unix
   :synopsis: Provide control for floating point exception handling.
.. moduleauthor:: Lee Busby <busby1@llnl.gov>
.. sectionauthor:: Lee Busby <busby1@llnl.gov>


.. note::

   The :mod:`fpectl` module is not built by default, and its usage is discouraged
   and may be dangerous except in the hands of experts.  See also the section
   :ref:`fpectl-limitations` on limitations for more details.

.. index:: single: IEEE-754

Most computers carry out floating point operations in conformance with the
so-called IEEE-754 standard. On any real computer, some floating point
operations produce results that cannot be expressed as a normal floating point
value. For example, try ::

   >>> import math
   >>> math.exp(1000)
   inf
   >>> math.exp(1000) / math.exp(1000)
   nan

(The example above will work on many platforms. DEC Alpha may be one exception.)
"Inf" is a special, non-numeric value in IEEE-754 that stands for "infinity",
and "nan" means "not a number." Note that, other than the non-numeric results,
nothing special happened when you asked Python to carry out those calculations.
That is in fact the default behaviour prescribed in the IEEE-754 standard, and
if it works for you, stop reading now.

In some circumstances, it would be better to raise an exception and stop
processing at the point where the faulty operation was attempted. The
:mod:`fpectl` module is for use in that situation. It provides control over
floating point units from several hardware manufacturers, allowing the user to
turn on the generation of :const:`SIGFPE` whenever any of the IEEE-754
exceptions Division by Zero, Overflow, or Invalid Operation occurs. In tandem
with a pair of wrapper macros that are inserted into the C code comprising your
python system, :const:`SIGFPE` is trapped and converted into the Python
:exc:`FloatingPointError` exception.

The :mod:`fpectl` module defines the following functions and may raise the given
exception:


.. function:: turnon_sigfpe()

   Turn on the generation of :const:`SIGFPE`, and set up an appropriate signal
   handler.


.. function:: turnoff_sigfpe()

   Reset default handling of floating point exceptions.


.. exception:: FloatingPointError

   After :func:`turnon_sigfpe` has been executed, a floating point operation that
   raises one of the IEEE-754 exceptions Division by Zero, Overflow, or Invalid
   operation will in turn raise this standard Python exception.


.. _fpectl-example:

Example
-------

The following example demonstrates how to start up and test operation of the
:mod:`fpectl` module. ::

   >>> import fpectl
   >>> import fpetest
   >>> fpectl.turnon_sigfpe()
   >>> fpetest.test()
   overflow        PASS
   FloatingPointError: Overflow

   div by 0        PASS
   FloatingPointError: Division by zero
     [ more output from test elided ]
   >>> import math
   >>> math.exp(1000)
   Traceback (most recent call last):
     File "<stdin>", line 1, in ?
   FloatingPointError: in math_1


.. _fpectl-limitations:

Limitations and other considerations
------------------------------------

Setting up a given processor to trap IEEE-754 floating point errors currently
requires custom code on a per-architecture basis. You may have to modify
:mod:`fpectl` to control your particular hardware.

Conversion of an IEEE-754 exception to a Python exception requires that the
wrapper macros ``PyFPE_START_PROTECT`` and ``PyFPE_END_PROTECT`` be inserted
into your code in an appropriate fashion.  Python itself has been modified to
support the :mod:`fpectl` module, but many other codes of interest to numerical
analysts have not.

The :mod:`fpectl` module is not thread-safe.


.. seealso::

   Some files in the source distribution may be interesting in learning more about
   how this module operates. The include file :source:`Include/pyfpe.h` discusses the
   implementation of this module at some length. :source:`Modules/fpetestmodule.c`
   gives several examples of use. Many additional examples can be found in
   :source:`Objects/floatobject.c`.

