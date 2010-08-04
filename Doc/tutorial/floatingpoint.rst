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

Stop at any finite number of bits, and you get an approximation.

On a typical machine running Python, there are 53 bits of precision available
for a Python float, so the value stored internally when you enter the decimal
number ``0.1`` is the binary fraction ::

   0.00011001100110011001100110011001100110011001100110011010

which is close to, but not exactly equal to, 1/10.

It's easy to forget that the stored value is an approximation to the original
decimal fraction, because of the way that floats are displayed at the
interpreter prompt.  Python only prints a decimal approximation to the true
decimal value of the binary approximation stored by the machine.  If Python
were to print the true decimal value of the binary approximation stored for
0.1, it would have to display ::

   >>> 0.1
   0.1000000000000000055511151231257827021181583404541015625

That is more digits than most people find useful, so Python keeps the number
of digits manageable by displaying a rounded value instead ::

   >>> 0.1
   0.1

It's important to realize that this is, in a real sense, an illusion: the value
in the machine is not exactly 1/10, you're simply rounding the *display* of the
true machine value.  This fact becomes apparent as soon as you try to do
arithmetic with these values ::

   >>> 0.1 + 0.2
   0.30000000000000004

Note that this is in the very nature of binary floating-point: this is not a
bug in Python, and it is not a bug in your code either.  You'll see the same
kind of thing in all languages that support your hardware's floating-point
arithmetic (although some languages may not *display* the difference by
default, or in all output modes).

Other surprises follow from this one.  For example, if you try to round the
value 2.675 to two decimal places, you get this ::

   >>> round(2.675, 2)
   2.67

The documentation for the built-in :func:`round` function says that it rounds
to the nearest value, rounding ties away from zero.  Since the decimal fraction
2.675 is exactly halfway between 2.67 and 2.68, you might expect the result
here to be (a binary approximation to) 2.68.  It's not, because when the
decimal string ``2.675`` is converted to a binary floating-point number, it's
again replaced with a binary approximation, whose exact value is ::

   2.67499999999999982236431605997495353221893310546875

Since this approximation is slightly closer to 2.67 than to 2.68, it's rounded
down.

If you're in a situation where you care which way your decimal halfway-cases
are rounded, you should consider using the :mod:`decimal` module.
Incidentally, the :mod:`decimal` module also provides a nice way to "see" the
exact value that's stored in any particular Python float ::

   >>> from decimal import Decimal
   >>> Decimal(2.675)
   Decimal('2.67499999999999982236431605997495353221893310546875')

Another consequence is that since 0.1 is not exactly 1/10, summing ten values
of 0.1 may not yield exactly 1.0, either::

   >>> sum = 0.0
   >>> for i in range(10):
   ...     sum += 0.1
   ...
   >>> sum
   0.9999999999999999

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
display of your final results to the number of decimal digits you expect.  For
fine control over how a float is displayed see the :meth:`str.format` method's
format specifiers in :ref:`formatstrings`.


.. _tut-fp-error:

Representation Error
====================

This section explains the "0.1" example in detail, and shows how you can
perform an exact analysis of cases like this yourself.  Basic familiarity with
binary floating-point representation is assumed.

:dfn:`Representation error` refers to the fact that some (most, actually)
decimal fractions cannot be represented exactly as binary (base 2) fractions.
This is the chief reason why Python (or Perl, C, C++, Java, Fortran, and many
others) often won't display the exact decimal number you expect::

   >>> 0.1 + 0.2
   0.30000000000000004

Why is that?  1/10 and 2/10 are not exactly representable as a binary
fraction. Almost all machines today (July 2010) use IEEE-754 floating point
arithmetic, and almost all platforms map Python floats to IEEE-754 "double
precision".  754 doubles contain 53 bits of precision, so on input the computer
strives to convert 0.1 to the closest fraction it can of the form *J*/2**\ *N*
where *J* is an integer containing exactly 53 bits.  Rewriting ::

   1 / 10 ~= J / (2**N)

as ::

   J ~= 2**N / 10

and recalling that *J* has exactly 53 bits (is ``>= 2**52`` but ``< 2**53``),
the best value for *N* is 56::

   >>> 2**52
   4503599627370496
   >>> 2**53
   9007199254740992
   >>> 2**56/10
   7205759403792793

That is, 56 is the only value for *N* that leaves *J* with exactly 53 bits.
The best possible value for *J* is then that quotient rounded::

   >>> q, r = divmod(2**56, 10)
   >>> r
   6

Since the remainder is more than half of 10, the best approximation is obtained
by rounding up::

   >>> q+1
   7205759403792794

Therefore the best possible approximation to 1/10 in 754 double precision is
that over 2\*\*56, or ::

   7205759403792794 / 72057594037927936

Note that since we rounded up, this is actually a little bit larger than 1/10;
if we had not rounded up, the quotient would have been a little bit smaller
than 1/10.  But in no case can it be *exactly* 1/10!

So the computer never "sees" 1/10:  what it sees is the exact fraction given
above, the best 754 double approximation it can get::

   >>> .1 * 2**56
   7205759403792794.0

If we multiply that fraction by 10\*\*30, we can see the (truncated) value of
its 30 most significant decimal digits::

   >>> 7205759403792794 * 10**30 // 2**56
   100000000000000005551115123125L

meaning that the exact number stored in the computer is approximately equal to
the decimal value 0.100000000000000005551115123125.  In versions prior to
Python 2.7 and Python 3.1, Python rounded this value to 17 significant digits,
giving '0.10000000000000001'.  In current versions, Python displays a value
based on the shortest decimal fraction that rounds correctly back to the true
binary value, resulting simply in '0.1'.
