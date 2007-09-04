.. _tut-fp-issues:

**************************************************
Floating Point Arithmetic:  Issues and Limitations
**************************************************

.. sectionauthor:: Tim Peters <tim_one@users.sourceforge.net>


Floating-point numbers are represented in computer hardware as base 2 (binary)
fractions.  For example, the decimal fraction ::

   0.125

has value 1/10 + 2/100 + 5/1000, and in the same way the binary fraction ::

   0.001

has value 0/2 + 0/4 + 1/8.  These two fractions have identical values, the only
real difference being that the first is written in base 10 fractional notation,
and the second in base 2.

Unfortunately, most decimal fractions cannot be represented exactly as binary
fractions.  A consequence is that, in general, the decimal floating-point
numbers you enter are only approximated by the binary floating-point numbers
actually stored in the machine.

The problem is easier to understand at first in base 10.  Consider the fraction
1/3.  You can approximate that as a base 10 fraction::

   0.3

or, better, ::

   0.33

or, better, ::

   0.333

and so on.  No matter how many digits you're willing to write down, the result
will never be exactly 1/3, but will be an increasingly better approximation of
1/3.

In the same way, no matter how many base 2 digits you're willing to use, the
decimal value 0.1 cannot be represented exactly as a base 2 fraction.  In base
2, 1/10 is the infinitely repeating fraction ::

   0.0001100110011001100110011001100110011001100110011...

Stop at any finite number of bits, and you get an approximation.  This is why
you see things like::

   >>> 0.1
   0.10000000000000001

On most machines today, that is what you'll see if you enter 0.1 at a Python
prompt.  You may not, though, because the number of bits used by the hardware to
store floating-point values can vary across machines, and Python only prints a
decimal approximation to the true decimal value of the binary approximation
stored by the machine.  On most machines, if Python were to print the true
decimal value of the binary approximation stored for 0.1, it would have to
display ::

   >>> 0.1
   0.1000000000000000055511151231257827021181583404541015625

instead!  The Python prompt uses the builtin :func:`repr` function to obtain a
string version of everything it displays.  For floats, ``repr(float)`` rounds
the true decimal value to 17 significant digits, giving ::

   0.10000000000000001

``repr(float)`` produces 17 significant digits because it turns out that's
enough (on most machines) so that ``eval(repr(x)) == x`` exactly for all finite
floats *x*, but rounding to 16 digits is not enough to make that true.

Note that this is in the very nature of binary floating-point: this is not a bug
in Python, and it is not a bug in your code either.  You'll see the same kind of
thing in all languages that support your hardware's floating-point arithmetic
(although some languages may not *display* the difference by default, or in all
output modes).

Python's builtin :func:`str` function produces only 12 significant digits, and
you may wish to use that instead.  It's unusual for ``eval(str(x))`` to
reproduce *x*, but the output may be more pleasant to look at::

   >>> print(str(0.1))
   0.1

It's important to realize that this is, in a real sense, an illusion: the value
in the machine is not exactly 1/10, you're simply rounding the *display* of the
true machine value.

Other surprises follow from this one.  For example, after seeing ::

   >>> 0.1
   0.10000000000000001

you may be tempted to use the :func:`round` function to chop it back to the
single digit you expect.  But that makes no difference::

   >>> round(0.1, 1)
   0.10000000000000001

The problem is that the binary floating-point value stored for "0.1" was already
the best possible binary approximation to 1/10, so trying to round it again
can't make it better:  it was already as good as it gets.

Another consequence is that since 0.1 is not exactly 1/10, summing ten values of
0.1 may not yield exactly 1.0, either::

   >>> sum = 0.0
   >>> for i in range(10):
   ...     sum += 0.1
   ...
   >>> sum
   0.99999999999999989

Binary floating-point arithmetic holds many surprises like this.  The problem
with "0.1" is explained in precise detail below, in the "Representation Error"
section.  See `The Perils of Floating Point <http://www.lahey.com/float.htm>`_
for a more complete account of other common surprises.

As that says near the end, "there are no easy answers."  Still, don't be unduly
wary of floating-point!  The errors in Python float operations are inherited
from the floating-point hardware, and on most machines are on the order of no
more than 1 part in 2\*\*53 per operation.  That's more than adequate for most
tasks, but you do need to keep in mind that it's not decimal arithmetic, and
that every float operation can suffer a new rounding error.

While pathological cases do exist, for most casual use of floating-point
arithmetic you'll see the result you expect in the end if you simply round the
display of your final results to the number of decimal digits you expect.
:func:`str` usually suffices, and for finer control see the discussion of
Python's ``%`` format operator: the ``%g``, ``%f`` and ``%e`` format codes
supply flexible and easy ways to round float results for display.

If you are a heavy user of floating point operations you should take a look
at the Numerical Python package and many other packages for mathematical and
statistical operations supplied by the SciPy project. See <http://scipy.org>.
 
.. _tut-fp-error:

Representation Error
====================

This section explains the "0.1" example in detail, and shows how you can perform
an exact analysis of cases like this yourself.  Basic familiarity with binary
floating-point representation is assumed.

:dfn:`Representation error` refers to the fact that some (most, actually)
decimal fractions cannot be represented exactly as binary (base 2) fractions.
This is the chief reason why Python (or Perl, C, C++, Java, Fortran, and many
others) often won't display the exact decimal number you expect::

   >>> 0.1
   0.10000000000000001

Why is that?  1/10 is not exactly representable as a binary fraction. Almost all
machines today (November 2000) use IEEE-754 floating point arithmetic, and
almost all platforms map Python floats to IEEE-754 "double precision".  754
doubles contain 53 bits of precision, so on input the computer strives to
convert 0.1 to the closest fraction it can of the form *J*/2\*\**N* where *J* is
an integer containing exactly 53 bits.  Rewriting ::

   1 / 10 ~= J / (2**N)

as ::

   J ~= 2**N / 10

and recalling that *J* has exactly 53 bits (is ``>= 2**52`` but ``< 2**53``),
the best value for *N* is 56::

   >>> 2**52
   4503599627370496L
   >>> 2**53
   9007199254740992L
   >>> 2**56/10
   7205759403792793L

That is, 56 is the only value for *N* that leaves *J* with exactly 53 bits.  The
best possible value for *J* is then that quotient rounded::

   >>> q, r = divmod(2**56, 10)
   >>> r
   6L

Since the remainder is more than half of 10, the best approximation is obtained
by rounding up::

   >>> q+1
   7205759403792794L

Therefore the best possible approximation to 1/10 in 754 double precision is
that over 2\*\*56, or ::

   7205759403792794 / 72057594037927936

Note that since we rounded up, this is actually a little bit larger than 1/10;
if we had not rounded up, the quotient would have been a little bit smaller than
1/10.  But in no case can it be *exactly* 1/10!

So the computer never "sees" 1/10:  what it sees is the exact fraction given
above, the best 754 double approximation it can get::

   >>> .1 * 2**56
   7205759403792794.0

If we multiply that fraction by 10\*\*30, we can see the (truncated) value of
its 30 most significant decimal digits::

   >>> 7205759403792794 * 10**30 / 2**56
   100000000000000005551115123125L

meaning that the exact number stored in the computer is approximately equal to
the decimal value 0.100000000000000005551115123125.  Rounding that to 17
significant digits gives the 0.10000000000000001 that Python displays (well,
will display on any 754-conforming platform that does best-possible input and
output conversions in its C library --- yours may not!).


