/*
 * Copyright (c) 2001 Python Software Foundation. All Rights Reserved.
 * Modified and extended by Stefan Krah.
 */


#ifndef DOCSTRINGS_H
#define DOCSTRINGS_H


#include "pymacro.h"


/******************************************************************************/
/*                                Module                                      */
/******************************************************************************/


PyDoc_STRVAR(doc__decimal,
"C decimal arithmetic module");

PyDoc_STRVAR(doc_getcontext,
"getcontext($module, /)\n--\n\n\
Get the current default context.\n\
\n");

PyDoc_STRVAR(doc_setcontext,
"setcontext($module, context, /)\n--\n\n\
Set a new default context.\n\
\n");

PyDoc_STRVAR(doc_localcontext,
"localcontext($module, /, ctx=None, **kwargs)\n--\n\n\
Return a context manager that will set the default context to a copy of ctx\n\
on entry to the with-statement and restore the previous default context when\n\
exiting the with-statement. If no context is specified, a copy of the current\n\
default context is used.\n\
\n");

PyDoc_STRVAR(doc_ieee_context,
"IEEEContext($module, bits, /)\n--\n\n\
Return a context object initialized to the proper values for one of the\n\
IEEE interchange formats.  The argument must be a multiple of 32 and less\n\
than IEEE_CONTEXT_MAX_BITS.\n\
\n");


/******************************************************************************/
/*                       Decimal Object and Methods                           */
/******************************************************************************/

PyDoc_STRVAR(doc_decimal,
"Decimal(value=\"0\", context=None)\n--\n\n\
Construct a new Decimal object. 'value' can be an integer, string, tuple,\n\
or another Decimal object. If no value is given, return Decimal('0'). The\n\
context does not affect the conversion and is only passed to determine if\n\
the InvalidOperation trap is active.\n\
\n");

PyDoc_STRVAR(doc_adjusted,
"adjusted($self, /)\n--\n\n\
Return the adjusted exponent of the number.  Defined as exp + digits - 1.\n\
\n");

PyDoc_STRVAR(doc_as_tuple,
"as_tuple($self, /)\n--\n\n\
Return a tuple representation of the number.\n\
\n");

PyDoc_STRVAR(doc_as_integer_ratio,
"as_integer_ratio($self, /)\n--\n\n\
Decimal.as_integer_ratio() -> (int, int)\n\
\n\
Return a pair of integers, whose ratio is exactly equal to the original\n\
Decimal and with a positive denominator. The ratio is in lowest terms.\n\
Raise OverflowError on infinities and a ValueError on NaNs.\n\
\n");

PyDoc_STRVAR(doc_canonical,
"canonical($self, /)\n--\n\n\
Return the canonical encoding of the argument.  Currently, the encoding\n\
of a Decimal instance is always canonical, so this operation returns its\n\
argument unchanged.\n\
\n");

PyDoc_STRVAR(doc_compare,
"compare($self, /, other, context=None)\n--\n\n\
Compare self to other.  Return a decimal value:\n\
\n\
    a or b is a NaN ==> Decimal('NaN')\n\
    a < b           ==> Decimal('-1')\n\
    a == b          ==> Decimal('0')\n\
    a > b           ==> Decimal('1')\n\
\n");

PyDoc_STRVAR(doc_compare_signal,
"compare_signal($self, /, other, context=None)\n--\n\n\
Identical to compare, except that all NaNs signal.\n\
\n");

PyDoc_STRVAR(doc_compare_total,
"compare_total($self, /, other, context=None)\n--\n\n\
Compare two operands using their abstract representation rather than\n\
their numerical value.  Similar to the compare() method, but the result\n\
gives a total ordering on Decimal instances.  Two Decimal instances with\n\
the same numeric value but different representations compare unequal\n\
in this ordering:\n\
\n\
    >>> Decimal('12.0').compare_total(Decimal('12'))\n\
    Decimal('-1')\n\
\n\
Quiet and signaling NaNs are also included in the total ordering. The result\n\
of this function is Decimal('0') if both operands have the same representation,\n\
Decimal('-1') if the first operand is lower in the total order than the second,\n\
and Decimal('1') if the first operand is higher in the total order than the\n\
second operand. See the specification for details of the total order.\n\
\n\
This operation is unaffected by context and is quiet: no flags are changed\n\
and no rounding is performed. As an exception, the C version may raise\n\
InvalidOperation if the second operand cannot be converted exactly.\n\
\n");

PyDoc_STRVAR(doc_compare_total_mag,
"compare_total_mag($self, /, other, context=None)\n--\n\n\
Compare two operands using their abstract representation rather than their\n\
value as in compare_total(), but ignoring the sign of each operand.\n\
\n\
x.compare_total_mag(y) is equivalent to x.copy_abs().compare_total(y.copy_abs()).\n\
\n\
This operation is unaffected by context and is quiet: no flags are changed\n\
and no rounding is performed. As an exception, the C version may raise\n\
InvalidOperation if the second operand cannot be converted exactly.\n\
\n");

PyDoc_STRVAR(doc_conjugate,
"conjugate($self, /)\n--\n\n\
Return self.\n\
\n");

PyDoc_STRVAR(doc_copy_abs,
"copy_abs($self, /)\n--\n\n\
Return the absolute value of the argument.  This operation is unaffected by\n\
context and is quiet: no flags are changed and no rounding is performed.\n\
\n");

PyDoc_STRVAR(doc_copy_negate,
"copy_negate($self, /)\n--\n\n\
Return the negation of the argument.  This operation is unaffected by context\n\
and is quiet: no flags are changed and no rounding is performed.\n\
\n");

PyDoc_STRVAR(doc_copy_sign,
"copy_sign($self, /, other, context=None)\n--\n\n\
Return a copy of the first operand with the sign set to be the same as the\n\
sign of the second operand. For example:\n\
\n\
    >>> Decimal('2.3').copy_sign(Decimal('-1.5'))\n\
    Decimal('-2.3')\n\
\n\
This operation is unaffected by context and is quiet: no flags are changed\n\
and no rounding is performed. As an exception, the C version may raise\n\
InvalidOperation if the second operand cannot be converted exactly.\n\
\n");

PyDoc_STRVAR(doc_exp,
"exp($self, /, context=None)\n--\n\n\
Return the value of the (natural) exponential function e**x at the given\n\
number.  The function always uses the ROUND_HALF_EVEN mode and the result\n\
is correctly rounded.\n\
\n");

PyDoc_STRVAR(doc_from_float,
"from_float($type, f, /)\n--\n\n\
Class method that converts a float to a decimal number, exactly.\n\
Since 0.1 is not exactly representable in binary floating point,\n\
Decimal.from_float(0.1) is not the same as Decimal('0.1').\n\
\n\
    >>> Decimal.from_float(0.1)\n\
    Decimal('0.1000000000000000055511151231257827021181583404541015625')\n\
    >>> Decimal.from_float(float('nan'))\n\
    Decimal('NaN')\n\
    >>> Decimal.from_float(float('inf'))\n\
    Decimal('Infinity')\n\
    >>> Decimal.from_float(float('-inf'))\n\
    Decimal('-Infinity')\n\
\n\
\n");

PyDoc_STRVAR(doc_from_number,
"from_number($type, number, /)\n--\n\n\
Class method that converts a real number to a decimal number, exactly.\n\
\n\
    >>> Decimal.from_number(314)              # int\n\
    Decimal('314')\n\
    >>> Decimal.from_number(0.1)              # float\n\
    Decimal('0.1000000000000000055511151231257827021181583404541015625')\n\
    >>> Decimal.from_number(Decimal('3.14'))  # another decimal instance\n\
    Decimal('3.14')\n\
\n\
\n");

PyDoc_STRVAR(doc_fma,
"fma($self, /, other, third, context=None)\n--\n\n\
Fused multiply-add.  Return self*other+third with no rounding of the\n\
intermediate product self*other.\n\
\n\
    >>> Decimal(2).fma(3, 5)\n\
    Decimal('11')\n\
\n\
\n");

PyDoc_STRVAR(doc_is_canonical,
"is_canonical($self, /)\n--\n\n\
Return True if the argument is canonical and False otherwise.  Currently,\n\
a Decimal instance is always canonical, so this operation always returns\n\
True.\n\
\n");

PyDoc_STRVAR(doc_is_finite,
"is_finite($self, /)\n--\n\n\
Return True if the argument is a finite number, and False if the argument\n\
is infinite or a NaN.\n\
\n");

PyDoc_STRVAR(doc_is_infinite,
"is_infinite($self, /)\n--\n\n\
Return True if the argument is either positive or negative infinity and\n\
False otherwise.\n\
\n");

PyDoc_STRVAR(doc_is_nan,
"is_nan($self, /)\n--\n\n\
Return True if the argument is a (quiet or signaling) NaN and False\n\
otherwise.\n\
\n");

PyDoc_STRVAR(doc_is_normal,
"is_normal($self, /, context=None)\n--\n\n\
Return True if the argument is a normal finite non-zero number with an\n\
adjusted exponent greater than or equal to Emin. Return False if the\n\
argument is zero, subnormal, infinite or a NaN.\n\
\n");

PyDoc_STRVAR(doc_is_qnan,
"is_qnan($self, /)\n--\n\n\
Return True if the argument is a quiet NaN, and False otherwise.\n\
\n");

PyDoc_STRVAR(doc_is_signed,
"is_signed($self, /)\n--\n\n\
Return True if the argument has a negative sign and False otherwise.\n\
Note that both zeros and NaNs can carry signs.\n\
\n");

PyDoc_STRVAR(doc_is_snan,
"is_snan($self, /)\n--\n\n\
Return True if the argument is a signaling NaN and False otherwise.\n\
\n");

PyDoc_STRVAR(doc_is_subnormal,
"is_subnormal($self, /, context=None)\n--\n\n\
Return True if the argument is subnormal, and False otherwise. A number is\n\
subnormal if it is non-zero, finite, and has an adjusted exponent less\n\
than Emin.\n\
\n");

PyDoc_STRVAR(doc_is_zero,
"is_zero($self, /)\n--\n\n\
Return True if the argument is a (positive or negative) zero and False\n\
otherwise.\n\
\n");

PyDoc_STRVAR(doc_ln,
"ln($self, /, context=None)\n--\n\n\
Return the natural (base e) logarithm of the operand. The function always\n\
uses the ROUND_HALF_EVEN mode and the result is correctly rounded.\n\
\n");

PyDoc_STRVAR(doc_log10,
"log10($self, /, context=None)\n--\n\n\
Return the base ten logarithm of the operand. The function always uses the\n\
ROUND_HALF_EVEN mode and the result is correctly rounded.\n\
\n");

PyDoc_STRVAR(doc_logb,
"logb($self, /, context=None)\n--\n\n\
For a non-zero number, return the adjusted exponent of the operand as a\n\
Decimal instance.  If the operand is a zero, then Decimal('-Infinity') is\n\
returned and the DivisionByZero condition is raised. If the operand is\n\
an infinity then Decimal('Infinity') is returned.\n\
\n");

PyDoc_STRVAR(doc_logical_and,
"logical_and($self, /, other, context=None)\n--\n\n\
Return the digit-wise 'and' of the two (logical) operands.\n\
\n");

PyDoc_STRVAR(doc_logical_invert,
"logical_invert($self, /, context=None)\n--\n\n\
Return the digit-wise inversion of the (logical) operand.\n\
\n");

PyDoc_STRVAR(doc_logical_or,
"logical_or($self, /, other, context=None)\n--\n\n\
Return the digit-wise 'or' of the two (logical) operands.\n\
\n");

PyDoc_STRVAR(doc_logical_xor,
"logical_xor($self, /, other, context=None)\n--\n\n\
Return the digit-wise 'exclusive or' of the two (logical) operands.\n\
\n");

PyDoc_STRVAR(doc_max,
"max($self, /, other, context=None)\n--\n\n\
Maximum of self and other.  If one operand is a quiet NaN and the other is\n\
numeric, the numeric operand is returned.\n\
\n");

PyDoc_STRVAR(doc_max_mag,
"max_mag($self, /, other, context=None)\n--\n\n\
Similar to the max() method, but the comparison is done using the absolute\n\
values of the operands.\n\
\n");

PyDoc_STRVAR(doc_min,
"min($self, /, other, context=None)\n--\n\n\
Minimum of self and other. If one operand is a quiet NaN and the other is\n\
numeric, the numeric operand is returned.\n\
\n");

PyDoc_STRVAR(doc_min_mag,
"min_mag($self, /, other, context=None)\n--\n\n\
Similar to the min() method, but the comparison is done using the absolute\n\
values of the operands.\n\
\n");

PyDoc_STRVAR(doc_next_minus,
"next_minus($self, /, context=None)\n--\n\n\
Return the largest number representable in the given context (or in the\n\
current default context if no context is given) that is smaller than the\n\
given operand.\n\
\n");

PyDoc_STRVAR(doc_next_plus,
"next_plus($self, /, context=None)\n--\n\n\
Return the smallest number representable in the given context (or in the\n\
current default context if no context is given) that is larger than the\n\
given operand.\n\
\n");

PyDoc_STRVAR(doc_next_toward,
"next_toward($self, /, other, context=None)\n--\n\n\
If the two operands are unequal, return the number closest to the first\n\
operand in the direction of the second operand.  If both operands are\n\
numerically equal, return a copy of the first operand with the sign set\n\
to be the same as the sign of the second operand.\n\
\n");

PyDoc_STRVAR(doc_normalize,
"normalize($self, /, context=None)\n--\n\n\
Normalize the number by stripping the rightmost trailing zeros and\n\
converting any result equal to Decimal('0') to Decimal('0e0').  Used\n\
for producing canonical values for members of an equivalence class.\n\
For example, Decimal('32.100') and Decimal('0.321000e+2') both normalize\n\
to the equivalent value Decimal('32.1').\n\
\n");

PyDoc_STRVAR(doc_number_class,
"number_class($self, /, context=None)\n--\n\n\
Return a string describing the class of the operand.  The returned value\n\
is one of the following ten strings:\n\
\n\
    * '-Infinity', indicating that the operand is negative infinity.\n\
    * '-Normal', indicating that the operand is a negative normal number.\n\
    * '-Subnormal', indicating that the operand is negative and subnormal.\n\
    * '-Zero', indicating that the operand is a negative zero.\n\
    * '+Zero', indicating that the operand is a positive zero.\n\
    * '+Subnormal', indicating that the operand is positive and subnormal.\n\
    * '+Normal', indicating that the operand is a positive normal number.\n\
    * '+Infinity', indicating that the operand is positive infinity.\n\
    * 'NaN', indicating that the operand is a quiet NaN (Not a Number).\n\
    * 'sNaN', indicating that the operand is a signaling NaN.\n\
\n\
\n");

PyDoc_STRVAR(doc_quantize,
"quantize($self, /, exp, rounding=None, context=None)\n--\n\n\
Return a value equal to the first operand after rounding and having the\n\
exponent of the second operand.\n\
\n\
    >>> Decimal('1.41421356').quantize(Decimal('1.000'))\n\
    Decimal('1.414')\n\
\n\
Unlike other operations, if the length of the coefficient after the quantize\n\
operation would be greater than precision, then an InvalidOperation is signaled.\n\
This guarantees that, unless there is an error condition, the quantized exponent\n\
is always equal to that of the right-hand operand.\n\
\n\
Also unlike other operations, quantize never signals Underflow, even if the\n\
result is subnormal and inexact.\n\
\n\
If the exponent of the second operand is larger than that of the first, then\n\
rounding may be necessary. In this case, the rounding mode is determined by the\n\
rounding argument if given, else by the given context argument; if neither\n\
argument is given, the rounding mode of the current thread's context is used.\n\
\n");

PyDoc_STRVAR(doc_radix,
"radix($self, /)\n--\n\n\
Return Decimal(10), the radix (base) in which the Decimal class does\n\
all its arithmetic. Included for compatibility with the specification.\n\
\n");

PyDoc_STRVAR(doc_remainder_near,
"remainder_near($self, /, other, context=None)\n--\n\n\
Return the remainder from dividing self by other.  This differs from\n\
self % other in that the sign of the remainder is chosen so as to minimize\n\
its absolute value. More precisely, the return value is self - n * other\n\
where n is the integer nearest to the exact value of self / other, and\n\
if two integers are equally near then the even one is chosen.\n\
\n\
If the result is zero then its sign will be the sign of self.\n\
\n");

PyDoc_STRVAR(doc_rotate,
"rotate($self, /, other, context=None)\n--\n\n\
Return the result of rotating the digits of the first operand by an amount\n\
specified by the second operand.  The second operand must be an integer in\n\
the range -precision through precision. The absolute value of the second\n\
operand gives the number of places to rotate. If the second operand is\n\
positive then rotation is to the left; otherwise rotation is to the right.\n\
The coefficient of the first operand is padded on the left with zeros to\n\
length precision if necessary. The sign and exponent of the first operand are\n\
unchanged.\n\
\n");

PyDoc_STRVAR(doc_same_quantum,
"same_quantum($self, /, other, context=None)\n--\n\n\
Test whether self and other have the same exponent or whether both are NaN.\n\
\n\
This operation is unaffected by context and is quiet: no flags are changed\n\
and no rounding is performed. As an exception, the C version may raise\n\
InvalidOperation if the second operand cannot be converted exactly.\n\
\n");

PyDoc_STRVAR(doc_scaleb,
"scaleb($self, /, other, context=None)\n--\n\n\
Return the first operand with the exponent adjusted the second.  Equivalently,\n\
return the first operand multiplied by 10**other. The second operand must be\n\
an integer.\n\
\n");

PyDoc_STRVAR(doc_shift,
"shift($self, /, other, context=None)\n--\n\n\
Return the result of shifting the digits of the first operand by an amount\n\
specified by the second operand.  The second operand must be an integer in\n\
the range -precision through precision. The absolute value of the second\n\
operand gives the number of places to shift. If the second operand is\n\
positive, then the shift is to the left; otherwise the shift is to the\n\
right. Digits shifted into the coefficient are zeros. The sign and exponent\n\
of the first operand are unchanged.\n\
\n");

PyDoc_STRVAR(doc_sqrt,
"sqrt($self, /, context=None)\n--\n\n\
Return the square root of the argument to full precision. The result is\n\
correctly rounded using the ROUND_HALF_EVEN rounding mode.\n\
\n");

PyDoc_STRVAR(doc_to_eng_string,
"to_eng_string($self, /, context=None)\n--\n\n\
Convert to an engineering-type string.  Engineering notation has an exponent\n\
which is a multiple of 3, so there are up to 3 digits left of the decimal\n\
place. For example, Decimal('123E+1') is converted to Decimal('1.23E+3').\n\
\n\
The value of context.capitals determines whether the exponent sign is lower\n\
or upper case. Otherwise, the context does not affect the operation.\n\
\n");

PyDoc_STRVAR(doc_to_integral,
"to_integral($self, /, rounding=None, context=None)\n--\n\n\
Identical to the to_integral_value() method.  The to_integral() name has been\n\
kept for compatibility with older versions.\n\
\n");

PyDoc_STRVAR(doc_to_integral_exact,
"to_integral_exact($self, /, rounding=None, context=None)\n--\n\n\
Round to the nearest integer, signaling Inexact or Rounded as appropriate if\n\
rounding occurs.  The rounding mode is determined by the rounding parameter\n\
if given, else by the given context. If neither parameter is given, then the\n\
rounding mode of the current default context is used.\n\
\n");

PyDoc_STRVAR(doc_to_integral_value,
"to_integral_value($self, /, rounding=None, context=None)\n--\n\n\
Round to the nearest integer without signaling Inexact or Rounded.  The\n\
rounding mode is determined by the rounding parameter if given, else by\n\
the given context. If neither parameter is given, then the rounding mode\n\
of the current default context is used.\n\
\n");


/******************************************************************************/
/*                       Context Object and Methods                           */
/******************************************************************************/

PyDoc_STRVAR(doc_context,
"Context(prec=None, rounding=None, Emin=None, Emax=None, capitals=None, clamp=None, flags=None, traps=None)\n--\n\n\
The context affects almost all operations and controls rounding,\n\
Over/Underflow, raising of exceptions and much more.  A new context\n\
can be constructed as follows:\n\
\n\
    >>> c = Context(prec=28, Emin=-425000000, Emax=425000000,\n\
    ...             rounding=ROUND_HALF_EVEN, capitals=1, clamp=1,\n\
    ...             traps=[InvalidOperation, DivisionByZero, Overflow],\n\
    ...             flags=[])\n\
    >>>\n\
\n\
\n");

#ifdef EXTRA_FUNCTIONALITY
PyDoc_STRVAR(doc_ctx_apply,
"apply($self, x, /)\n--\n\n\
Apply self to Decimal x.\n\
\n");
#endif

PyDoc_STRVAR(doc_ctx_clear_flags,
"clear_flags($self, /)\n--\n\n\
Reset all flags to False.\n\
\n");

PyDoc_STRVAR(doc_ctx_clear_traps,
"clear_traps($self, /)\n--\n\n\
Set all traps to False.\n\
\n");

PyDoc_STRVAR(doc_ctx_copy,
"copy($self, /)\n--\n\n\
Return a duplicate of the context with all flags cleared.\n\
\n");

PyDoc_STRVAR(doc_ctx_copy_decimal,
"copy_decimal($self, x, /)\n--\n\n\
Return a copy of Decimal x.\n\
\n");

PyDoc_STRVAR(doc_ctx_create_decimal,
"create_decimal($self, num=\"0\", /)\n--\n\n\
Create a new Decimal instance from num, using self as the context. Unlike the\n\
Decimal constructor, this function observes the context limits.\n\
\n");

PyDoc_STRVAR(doc_ctx_create_decimal_from_float,
"create_decimal_from_float($self, f, /)\n--\n\n\
Create a new Decimal instance from float f.  Unlike the Decimal.from_float()\n\
class method, this function observes the context limits.\n\
\n");

PyDoc_STRVAR(doc_ctx_Etiny,
"Etiny($self, /)\n--\n\n\
Return a value equal to Emin - prec + 1, which is the minimum exponent value\n\
for subnormal results.  When underflow occurs, the exponent is set to Etiny.\n\
\n");

PyDoc_STRVAR(doc_ctx_Etop,
"Etop($self, /)\n--\n\n\
Return a value equal to Emax - prec + 1.  This is the maximum exponent\n\
if the _clamp field of the context is set to 1 (IEEE clamp mode).  Etop()\n\
must not be negative.\n\
\n");

PyDoc_STRVAR(doc_ctx_abs,
"abs($self, x, /)\n--\n\n\
Return the absolute value of x.\n\
\n");

PyDoc_STRVAR(doc_ctx_add,
"add($self, x, y, /)\n--\n\n\
Return the sum of x and y.\n\
\n");

PyDoc_STRVAR(doc_ctx_canonical,
"canonical($self, x, /)\n--\n\n\
Return a new instance of x.\n\
\n");

PyDoc_STRVAR(doc_ctx_compare,
"compare($self, x, y, /)\n--\n\n\
Compare x and y numerically.\n\
\n");

PyDoc_STRVAR(doc_ctx_compare_signal,
"compare_signal($self, x, y, /)\n--\n\n\
Compare x and y numerically.  All NaNs signal.\n\
\n");

PyDoc_STRVAR(doc_ctx_compare_total,
"compare_total($self, x, y, /)\n--\n\n\
Compare x and y using their abstract representation.\n\
\n");

PyDoc_STRVAR(doc_ctx_compare_total_mag,
"compare_total_mag($self, x, y, /)\n--\n\n\
Compare x and y using their abstract representation, ignoring sign.\n\
\n");

PyDoc_STRVAR(doc_ctx_copy_abs,
"copy_abs($self, x, /)\n--\n\n\
Return a copy of x with the sign set to 0.\n\
\n");

PyDoc_STRVAR(doc_ctx_copy_negate,
"copy_negate($self, x, /)\n--\n\n\
Return a copy of x with the sign inverted.\n\
\n");

PyDoc_STRVAR(doc_ctx_copy_sign,
"copy_sign($self, x, y, /)\n--\n\n\
Copy the sign from y to x.\n\
\n");

PyDoc_STRVAR(doc_ctx_divide,
"divide($self, x, y, /)\n--\n\n\
Return x divided by y.\n\
\n");

PyDoc_STRVAR(doc_ctx_divide_int,
"divide_int($self, x, y, /)\n--\n\n\
Return x divided by y, truncated to an integer.\n\
\n");

PyDoc_STRVAR(doc_ctx_divmod,
"divmod($self, x, y, /)\n--\n\n\
Return quotient and remainder of the division x / y.\n\
\n");

PyDoc_STRVAR(doc_ctx_exp,
"exp($self, x, /)\n--\n\n\
Return e ** x.\n\
\n");

PyDoc_STRVAR(doc_ctx_fma,
"fma($self, x, y, z, /)\n--\n\n\
Return x multiplied by y, plus z.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_canonical,
"is_canonical($self, x, /)\n--\n\n\
Return True if x is canonical, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_finite,
"is_finite($self, x, /)\n--\n\n\
Return True if x is finite, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_infinite,
"is_infinite($self, x, /)\n--\n\n\
Return True if x is infinite, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_nan,
"is_nan($self, x, /)\n--\n\n\
Return True if x is a qNaN or sNaN, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_normal,
"is_normal($self, x, /)\n--\n\n\
Return True if x is a normal number, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_qnan,
"is_qnan($self, x, /)\n--\n\n\
Return True if x is a quiet NaN, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_signed,
"is_signed($self, x, /)\n--\n\n\
Return True if x is negative, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_snan,
"is_snan($self, x, /)\n--\n\n\
Return True if x is a signaling NaN, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_subnormal,
"is_subnormal($self, x, /)\n--\n\n\
Return True if x is subnormal, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_zero,
"is_zero($self, x, /)\n--\n\n\
Return True if x is a zero, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_ln,
"ln($self, x, /)\n--\n\n\
Return the natural (base e) logarithm of x.\n\
\n");

PyDoc_STRVAR(doc_ctx_log10,
"log10($self, x, /)\n--\n\n\
Return the base 10 logarithm of x.\n\
\n");

PyDoc_STRVAR(doc_ctx_logb,
"logb($self, x, /)\n--\n\n\
Return the exponent of the magnitude of the operand's MSD.\n\
\n");

PyDoc_STRVAR(doc_ctx_logical_and,
"logical_and($self, x, y, /)\n--\n\n\
Digit-wise and of x and y.\n\
\n");

PyDoc_STRVAR(doc_ctx_logical_invert,
"logical_invert($self, x, /)\n--\n\n\
Invert all digits of x.\n\
\n");

PyDoc_STRVAR(doc_ctx_logical_or,
"logical_or($self, x, y, /)\n--\n\n\
Digit-wise or of x and y.\n\
\n");

PyDoc_STRVAR(doc_ctx_logical_xor,
"logical_xor($self, x, y, /)\n--\n\n\
Digit-wise xor of x and y.\n\
\n");

PyDoc_STRVAR(doc_ctx_max,
"max($self, x, y, /)\n--\n\n\
Compare the values numerically and return the maximum.\n\
\n");

PyDoc_STRVAR(doc_ctx_max_mag,
"max_mag($self, x, y, /)\n--\n\n\
Compare the values numerically with their sign ignored.\n\
\n");

PyDoc_STRVAR(doc_ctx_min,
"min($self, x, y, /)\n--\n\n\
Compare the values numerically and return the minimum.\n\
\n");

PyDoc_STRVAR(doc_ctx_min_mag,
"min_mag($self, x, y, /)\n--\n\n\
Compare the values numerically with their sign ignored.\n\
\n");

PyDoc_STRVAR(doc_ctx_minus,
"minus($self, x, /)\n--\n\n\
Minus corresponds to the unary prefix minus operator in Python, but applies\n\
the context to the result.\n\
\n");

PyDoc_STRVAR(doc_ctx_multiply,
"multiply($self, x, y, /)\n--\n\n\
Return the product of x and y.\n\
\n");

PyDoc_STRVAR(doc_ctx_next_minus,
"next_minus($self, x, /)\n--\n\n\
Return the largest representable number smaller than x.\n\
\n");

PyDoc_STRVAR(doc_ctx_next_plus,
"next_plus($self, x, /)\n--\n\n\
Return the smallest representable number larger than x.\n\
\n");

PyDoc_STRVAR(doc_ctx_next_toward,
"next_toward($self, x, y, /)\n--\n\n\
Return the number closest to x, in the direction towards y.\n\
\n");

PyDoc_STRVAR(doc_ctx_normalize,
"normalize($self, x, /)\n--\n\n\
Reduce x to its simplest form. Alias for reduce(x).\n\
\n");

PyDoc_STRVAR(doc_ctx_number_class,
"number_class($self, x, /)\n--\n\n\
Return an indication of the class of x.\n\
\n");

PyDoc_STRVAR(doc_ctx_plus,
"plus($self, x, /)\n--\n\n\
Plus corresponds to the unary prefix plus operator in Python, but applies\n\
the context to the result.\n\
\n");

PyDoc_STRVAR(doc_ctx_power,
"power($self, /, a, b, modulo=None)\n--\n\n\
Compute a**b. If 'a' is negative, then 'b' must be integral. The result\n\
will be inexact unless 'a' is integral and the result is finite and can\n\
be expressed exactly in 'precision' digits.  In the Python version the\n\
result is always correctly rounded, in the C version the result is almost\n\
always correctly rounded.\n\
\n\
If modulo is given, compute (a**b) % modulo. The following restrictions\n\
hold:\n\
\n\
    * all three arguments must be integral\n\
    * 'b' must be nonnegative\n\
    * at least one of 'a' or 'b' must be nonzero\n\
    * modulo must be nonzero and less than 10**prec in absolute value\n\
\n\
\n");

PyDoc_STRVAR(doc_ctx_quantize,
"quantize($self, x, y, /)\n--\n\n\
Return a value equal to x (rounded), having the exponent of y.\n\
\n");

PyDoc_STRVAR(doc_ctx_radix,
"radix($self, /)\n--\n\n\
Return 10.\n\
\n");

PyDoc_STRVAR(doc_ctx_remainder,
"remainder($self, x, y, /)\n--\n\n\
Return the remainder from integer division.  The sign of the result,\n\
if non-zero, is the same as that of the original dividend.\n\
\n");

PyDoc_STRVAR(doc_ctx_remainder_near,
"remainder_near($self, x, y, /)\n--\n\n\
Return x - y * n, where n is the integer nearest the exact value of x / y\n\
(if the result is 0 then its sign will be the sign of x).\n\
\n");

PyDoc_STRVAR(doc_ctx_rotate,
"rotate($self, x, y, /)\n--\n\n\
Return a copy of x, rotated by y places.\n\
\n");

PyDoc_STRVAR(doc_ctx_same_quantum,
"same_quantum($self, x, y, /)\n--\n\n\
Return True if the two operands have the same exponent.\n\
\n");

PyDoc_STRVAR(doc_ctx_scaleb,
"scaleb($self, x, y, /)\n--\n\n\
Return the first operand after adding the second value to its exp.\n\
\n");

PyDoc_STRVAR(doc_ctx_shift,
"shift($self, x, y, /)\n--\n\n\
Return a copy of x, shifted by y places.\n\
\n");

PyDoc_STRVAR(doc_ctx_sqrt,
"sqrt($self, x, /)\n--\n\n\
Square root of a non-negative number to context precision.\n\
\n");

PyDoc_STRVAR(doc_ctx_subtract,
"subtract($self, x, y, /)\n--\n\n\
Return the difference between x and y.\n\
\n");

PyDoc_STRVAR(doc_ctx_to_eng_string,
"to_eng_string($self, x, /)\n--\n\n\
Convert a number to a string, using engineering notation.\n\
\n");

PyDoc_STRVAR(doc_ctx_to_integral,
"to_integral($self, x, /)\n--\n\n\
Identical to to_integral_value(x).\n\
\n");

PyDoc_STRVAR(doc_ctx_to_integral_exact,
"to_integral_exact($self, x, /)\n--\n\n\
Round to an integer. Signal if the result is rounded or inexact.\n\
\n");

PyDoc_STRVAR(doc_ctx_to_integral_value,
"to_integral_value($self, x, /)\n--\n\n\
Round to an integer.\n\
\n");

PyDoc_STRVAR(doc_ctx_to_sci_string,
"to_sci_string($self, x, /)\n--\n\n\
Convert a number to a string using scientific notation.\n\
\n");


#endif /* DOCSTRINGS_H */



