/*
 * Copyright (c) 2001-2012 Python Software Foundation. All Rights Reserved.
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

PyDoc_STRVAR(doc_getcontext,"\n\
getcontext() - Get the current default context.\n\
\n");

PyDoc_STRVAR(doc_setcontext,"\n\
setcontext(c) - Set a new default context.\n\
\n");

PyDoc_STRVAR(doc_localcontext,"\n\
localcontext(c) - Return a context manager that will set the default context\n\
to a copy of c on entry to the with-statement and restore the previous default\n\
context when exiting the with-statement. If no context is specified, a copy of\n\
the current default context is used.\n\
\n");

#ifdef EXTRA_FUNCTIONALITY
PyDoc_STRVAR(doc_ieee_context,"\n\
IEEEContext(bits) - Return a context object initialized to the proper values for\n\
one of the IEEE interchange formats. The argument must be a multiple of 32 and\n\
less than IEEE_CONTEXT_MAX_BITS. For the most common values, the constants\n\
DECIMAL32, DECIMAL64 and DECIMAL128 are provided.\n\
\n");
#endif


/******************************************************************************/
/*                       Decimal Object and Methods                           */
/******************************************************************************/

PyDoc_STRVAR(doc_decimal,"\n\
Decimal([value[, context]]): Construct a new Decimal object from value.\n\
\n\
value can be an integer, string, tuple, or another Decimal object.\n\
If no value is given, return Decimal('0'). The context does not affect\n\
the conversion and is only passed to determine if the InvalidOperation\n\
trap is active.\n\
\n");

PyDoc_STRVAR(doc_adjusted,"\n\
adjusted() - Return the adjusted exponent of the number.\n\
\n\
Defined as exp + digits - 1.\n\
\n");

PyDoc_STRVAR(doc_as_tuple,"\n\
as_tuple() - Return a tuple representation of the number.\n\
\n");

PyDoc_STRVAR(doc_canonical,"\n\
canonical() - Return the canonical encoding of the argument. Currently,\n\
the encoding of a Decimal instance is always canonical, so this operation\n\
returns its argument unchanged.\n\
\n");

PyDoc_STRVAR(doc_compare,"\n\
compare(other[, context]) - Compare self to other. Return a decimal value:\n\
\n\
    a or b is a NaN ==> Decimal('NaN')\n\
    a < b           ==> Decimal('-1')\n\
    a == b          ==> Decimal('0')\n\
    a > b           ==> Decimal('1')\n\
\n");

PyDoc_STRVAR(doc_compare_signal,"\n\
compare_signal(other[, context]) - Identical to compare, except that\n\
all NaNs signal.\n\
\n");

PyDoc_STRVAR(doc_compare_total,"\n\
compare_total(other) - Compare two operands using their abstract representation\n\
rather than their numerical value. Similar to the compare() method, but the\n\
result gives a total ordering on Decimal instances. Two Decimal instances with\n\
the same numeric value but different representations compare unequal in this\n\
ordering:\n\
\n\
    >>> Decimal('12.0').compare_total(Decimal('12'))\n\
    Decimal('-1')\n\
\n\
Quiet and signaling NaNs are also included in the total ordering. The result\n\
of this function is Decimal('0') if both operands have the same representation,\n\
Decimal('-1') if the first operand is lower in the total order than the second,\n\
and Decimal('1') if the first operand is higher in the total order than the\n\
second operand. See the specification for details of the total order.\n\
\n");

PyDoc_STRVAR(doc_compare_total_mag,"\n\
compare_total_mag(other) - Compare two operands using their abstract\n\
representation rather than their value as in compare_total(), but\n\
ignoring the sign of each operand.  x.compare_total_mag(y) is\n\
equivalent to x.copy_abs().compare_total(y.copy_abs()).\n\
\n");

PyDoc_STRVAR(doc_conjugate,"\n\
conjugate() - Return self.\n\
\n");

PyDoc_STRVAR(doc_copy_abs,"\n\
copy_abs() - Return the absolute value of the argument. This operation\n\
is unaffected by the context and is quiet: no flags are changed and no\n\
rounding is performed.\n\
\n");

PyDoc_STRVAR(doc_copy_negate,"\n\
copy_negate() - Return the negation of the argument. This operation is\n\
unaffected by the context and is quiet: no flags are changed and no\n\
rounding is performed.\n\
\n");

PyDoc_STRVAR(doc_copy_sign,"\n\
copy_sign(other) - Return a copy of the first operand with the sign set\n\
to be the same as the sign of the second operand. For example:\n\
\n\
    >>> Decimal('2.3').copy_sign(Decimal('-1.5'))\n\
    Decimal('-2.3')\n\
\n\
This operation is unaffected by the context and is quiet: no flags are\n\
changed and no rounding is performed.\n\
\n");

PyDoc_STRVAR(doc_exp,"\n\
exp([context]) - Return the value of the (natural) exponential function e**x\n\
at the given number. The function always uses the ROUND_HALF_EVEN mode and\n\
the result is correctly rounded.\n\
\n");

PyDoc_STRVAR(doc_from_float,"\n\
from_float(f) - Class method that converts a float to a decimal number, exactly.\n\
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

PyDoc_STRVAR(doc_fma,"\n\
fma(other, third[, context]) - Fused multiply-add. Return self*other+third\n\
with no rounding of the intermediate product self*other.\n\
\n\
    >>> Decimal(2).fma(3, 5)\n\
    Decimal('11')\n\
\n\
\n");

PyDoc_STRVAR(doc_is_canonical,"\n\
is_canonical() - Return True if the argument is canonical and False otherwise.\n\
Currently, a Decimal instance is always canonical, so this operation always\n\
returns True.\n\
\n");

PyDoc_STRVAR(doc_is_finite,"\n\
is_finite() - Return True if the argument is a finite number, and False if the\n\
argument is infinite or a NaN.\n\
\n");

PyDoc_STRVAR(doc_is_infinite,"\n\
is_infinite() - Return True if the argument is either positive or negative\n\
infinity and False otherwise.\n\
\n");

PyDoc_STRVAR(doc_is_nan,"\n\
is_nan() - Return True if the argument is a (quiet or signaling) NaN and\n\
False otherwise.\n\
\n");

PyDoc_STRVAR(doc_is_normal,"\n\
is_normal([context]) - Return True if the argument is a normal finite non-zero\n\
number with an adjusted exponent greater than or equal to Emin. Return False\n\
if the argument is zero, subnormal, infinite or a NaN.\n\
\n");

PyDoc_STRVAR(doc_is_qnan,"\n\
is_qnan() - Return True if the argument is a quiet NaN, and False otherwise.\n\
\n");

PyDoc_STRVAR(doc_is_signed,"\n\
is_signed() - Return True if the argument has a negative sign and\n\
False otherwise. Note that both zeros and NaNs can carry signs.\n\
\n");

PyDoc_STRVAR(doc_is_snan,"\n\
is_snan() - Return True if the argument is a signaling NaN and False otherwise.\n\
\n");

PyDoc_STRVAR(doc_is_subnormal,"\n\
is_subnormal([context]) - Return True if the argument is subnormal, and False\n\
otherwise. A number is subnormal if it is non-zero, finite, and has an\n\
adjusted exponent less than Emin.\n\
\n");

PyDoc_STRVAR(doc_is_zero,"\n\
is_zero() - Return True if the argument is a (positive or negative) zero and\n\
False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ln,"\n\
ln([context]) - Return the natural (base e) logarithm of the operand.\n\
The function always uses the ROUND_HALF_EVEN mode and the result is\n\
correctly rounded.\n\
\n");

PyDoc_STRVAR(doc_log10,"\n\
log10([context]) - Return the base ten logarithm of the operand.\n\
The function always uses the ROUND_HALF_EVEN mode and the result is\n\
correctly rounded.\n\
\n");

PyDoc_STRVAR(doc_logb,"\n\
logb([context]) - For a non-zero number, return the adjusted exponent\n\
of the operand as a Decimal instance. If the operand is a zero, then\n\
Decimal('-Infinity') is returned and the DivisionByZero condition is\n\
raised. If the operand is an infinity then Decimal('Infinity') is returned.\n\
\n");

PyDoc_STRVAR(doc_logical_and,"\n\
logical_and(other[, context]) - Return the digit-wise and of the two\n\
(logical) operands.\n\
\n");

PyDoc_STRVAR(doc_logical_invert,"\n\
logical_invert([context]) - Return the digit-wise inversion of the\n\
(logical) operand.\n\
\n");

PyDoc_STRVAR(doc_logical_or,"\n\
logical_or(other[, context]) - Return the digit-wise or of the two\n\
(logical) operands.\n\
\n");

PyDoc_STRVAR(doc_logical_xor,"\n\
logical_xor(other[, context]) - Return the digit-wise exclusive or of the\n\
two (logical) operands.\n\
\n");

PyDoc_STRVAR(doc_max,"\n\
max(other[, context]) - Maximum of self and other. If one operand is a quiet\n\
NaN and the other is numeric, the numeric operand is returned.\n\
\n");

PyDoc_STRVAR(doc_max_mag,"\n\
max_mag(other[, context]) - Similar to the max() method, but the comparison is\n\
done using the absolute values of the operands.\n\
\n");

PyDoc_STRVAR(doc_min,"\n\
min(other[, context]) - Minimum of self and other. If one operand is a quiet\n\
NaN and the other is numeric, the numeric operand is returned.\n\
\n");

PyDoc_STRVAR(doc_min_mag,"\n\
min_mag(other[, context]) - Similar to the min() method, but the comparison is\n\
done using the absolute values of the operands.\n\
\n");

PyDoc_STRVAR(doc_next_minus,"\n\
next_minus([context]) - Return the largest number representable in the given\n\
context (or in the current default context if no context is given) that is\n\
smaller than the given operand.\n\
\n");

PyDoc_STRVAR(doc_next_plus,"\n\
next_plus([context]) - Return the smallest number representable in the given\n\
context (or in the current default context if no context is given) that is\n\
larger than the given operand.\n\
\n");

PyDoc_STRVAR(doc_next_toward,"\n\
next_toward(other[, context]) - If the two operands are unequal, return the\n\
number closest to the first operand in the direction of the second operand.\n\
If both operands are numerically equal, return a copy of the first operand\n\
with the sign set to be the same as the sign of the second operand.\n\
\n");

PyDoc_STRVAR(doc_normalize,"\n\
normalize([context]) - Normalize the number by stripping the rightmost trailing\n\
zeros and converting any result equal to Decimal('0') to Decimal('0e0'). Used\n\
for producing canonical values for members of an equivalence class. For example,\n\
Decimal('32.100') and Decimal('0.321000e+2') both normalize to the equivalent\n\
value Decimal('32.1').\n\
\n");

PyDoc_STRVAR(doc_number_class,"\n\
number_class([context]) - Return a string describing the class of the operand.\n\
The returned value is one of the following ten strings:\n\
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

PyDoc_STRVAR(doc_quantize,"\n\
quantize(exp[, rounding[, context]]) - Return a value equal to the first\n\
operand after rounding and having the exponent of the second operand.\n\
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

PyDoc_STRVAR(doc_radix,"\n\
radix() - Return Decimal(10), the radix (base) in which the Decimal class does\n\
all its arithmetic. Included for compatibility with the specification.\n\
\n");

PyDoc_STRVAR(doc_remainder_near,"\n\
remainder_near(other[, context]) - Compute the modulo as either a positive\n\
or negative value depending on which is closest to zero. For instance,\n\
Decimal(10).remainder_near(6) returns Decimal('-2'), which is closer to zero\n\
than Decimal('4').\n\
\n\
If both are equally close, the one chosen will have the same sign as self.\n\
\n");

PyDoc_STRVAR(doc_rotate,"\n\
rotate(other[, context]) - Return the result of rotating the digits of the\n\
first operand by an amount specified by the second operand. The second operand\n\
must be an integer in the range -precision through precision. The absolute\n\
value of the second operand gives the number of places to rotate. If the second\n\
operand is positive then rotation is to the left; otherwise rotation is to the\n\
right. The coefficient of the first operand is padded on the left with zeros to\n\
length precision if necessary. The sign and exponent of the first operand are\n\
unchanged.\n\
\n");

PyDoc_STRVAR(doc_same_quantum,"\n\
same_quantum(other[, context]) - Test whether self and other have the\n\
same exponent or whether both are NaN.\n\
\n");

PyDoc_STRVAR(doc_scaleb,"\n\
scaleb(other[, context]) - Return the first operand with the exponent adjusted\n\
the second. Equivalently, return the first operand multiplied by 10**other.\n\
The second operand must be an integer.\n\
\n");

PyDoc_STRVAR(doc_shift,"\n\
shift(other[, context]) - Return the result of shifting the digits of\n\
the first operand by an amount specified by the second operand. The second\n\
operand must be an integer in the range -precision through precision. The\n\
absolute value of the second operand gives the number of places to shift.\n\
If the second operand is positive, then the shift is to the left; otherwise\n\
the shift is to the right. Digits shifted into the coefficient are zeros.\n\
The sign and exponent of the first operand are unchanged.\n\
\n");

PyDoc_STRVAR(doc_sqrt,"\n\
sqrt([context]) - Return the square root of the argument to full precision.\n\
The result is correctly rounded using the ROUND_HALF_EVEN rounding mode.\n\
\n");

PyDoc_STRVAR(doc_to_eng_string,"\n\
to_eng_string([context]) - Convert to an engineering-type string.\n\
Engineering notation has an exponent which is a multiple of 3, so\n\
there are up to 3 digits left of the decimal place. For example,\n\
Decimal('123E+1') is converted to Decimal('1.23E+3')\n\
\n");

PyDoc_STRVAR(doc_to_integral,"\n\
to_integral([rounding[, context]]) - Identical to the to_integral_value()\n\
method. The to_integral name has been kept for compatibility with older\n\
versions.\n\
\n");

PyDoc_STRVAR(doc_to_integral_exact,"\n\
to_integral_exact([rounding[, context]]) - Round to the nearest integer,\n\
signaling Inexact or Rounded as appropriate if rounding occurs. The rounding\n\
mode is determined by the rounding parameter if given, else by the given\n\
context. If neither parameter is given, then the rounding mode of the current\n\
default context is used.\n\
\n");

PyDoc_STRVAR(doc_to_integral_value,"\n\
to_integral_value([rounding[, context]]) - Round to the nearest integer without\n\
signaling Inexact or Rounded. The rounding mode is determined by the rounding\n\
parameter if given, else by the given context. If neither parameter is given,\n\
then the rounding mode of the current default context is used.\n\
\n");


/******************************************************************************/
/*                       Context Object and Methods                           */
/******************************************************************************/

PyDoc_STRVAR(doc_context,"\n\
The context affects almost all operations and controls rounding,\n\
Over/Underflow, raising of exceptions and much more. A new context\n\
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
PyDoc_STRVAR(doc_ctx_apply,"\n\
apply(x) - Apply self to Decimal x.\n\
\n");
#endif

PyDoc_STRVAR(doc_ctx_clear_flags,"\n\
clear_flags() - Reset all flags to False.\n\
\n");

PyDoc_STRVAR(doc_ctx_clear_traps,"\n\
clear_traps() - Set all traps to False.\n\
\n");

PyDoc_STRVAR(doc_ctx_copy,"\n\
copy() - Return a duplicate of the context with all flags cleared.\n\
\n");

PyDoc_STRVAR(doc_ctx_copy_decimal,"\n\
copy_decimal(x) - Return a copy of Decimal x.\n\
\n");

PyDoc_STRVAR(doc_ctx_create_decimal,"\n\
create_decimal(x) - Create a new Decimal instance from x, using self as the\n\
context. Unlike the Decimal constructor, this function observes the context\n\
limits.\n\
\n");

PyDoc_STRVAR(doc_ctx_create_decimal_from_float,"\n\
create_decimal_from_float(f) - Create a new Decimal instance from float f.\n\
Unlike the Decimal.from_float() class method, this function observes the\n\
context limits.\n\
\n");

PyDoc_STRVAR(doc_ctx_Etiny,"\n\
Etiny() - Return a value equal to Emin - prec + 1, which is the minimum\n\
exponent value for subnormal results. When underflow occurs, the exponent\n\
is set to Etiny.\n\
\n");

PyDoc_STRVAR(doc_ctx_Etop,"\n\
Etop() - Return a value equal to Emax - prec + 1. This is the maximum exponent\n\
if the _clamp field of the context is set to 1 (IEEE clamp mode). Etop() must\n\
not be negative.\n\
\n");

PyDoc_STRVAR(doc_ctx_abs,"\n\
abs(x) - Return the absolute value of x.\n\
\n");

PyDoc_STRVAR(doc_ctx_add,"\n\
add(x, y) - Return the sum of x and y.\n\
\n");

PyDoc_STRVAR(doc_ctx_canonical,"\n\
canonical(x) - Return a new instance of x.\n\
\n");

PyDoc_STRVAR(doc_ctx_compare,"\n\
compare(x, y) - Compare x and y numerically.\n\
\n");

PyDoc_STRVAR(doc_ctx_compare_signal,"\n\
compare_signal(x, y) - Compare x and y numerically. All NaNs signal.\n\
\n");

PyDoc_STRVAR(doc_ctx_compare_total,"\n\
compare_total(x, y) - Compare x and y using their abstract representation.\n\
\n");

PyDoc_STRVAR(doc_ctx_compare_total_mag,"\n\
compare_total_mag(x, y) - Compare x and y using their abstract representation,\n\
ignoring sign.\n\
\n");

PyDoc_STRVAR(doc_ctx_copy_abs,"\n\
copy_abs(x) - Return a copy of x with the sign set to 0.\n\
\n");

PyDoc_STRVAR(doc_ctx_copy_negate,"\n\
copy_negate(x) - Return a copy of x with the sign inverted.\n\
\n");

PyDoc_STRVAR(doc_ctx_copy_sign,"\n\
copy_sign(x, y) - Copy the sign from y to x.\n\
\n");

PyDoc_STRVAR(doc_ctx_divide,"\n\
divide(x, y) - Return x divided by y.\n\
\n");

PyDoc_STRVAR(doc_ctx_divide_int,"\n\
divide_int(x, y) - Return x divided by y, truncated to an integer.\n\
\n");

PyDoc_STRVAR(doc_ctx_divmod,"\n\
divmod(x, y) - Return quotient and remainder of the division x / y.\n\
\n");

PyDoc_STRVAR(doc_ctx_exp,"\n\
exp(x) - Return e ** x.\n\
\n");

PyDoc_STRVAR(doc_ctx_fma,"\n\
fma(x, y, z) - Return x multiplied by y, plus z.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_canonical,"\n\
is_canonical(x) - Return True if x is canonical, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_finite,"\n\
is_finite(x) - Return True if x is finite, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_infinite,"\n\
is_infinite(x) - Return True if x is infinite, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_nan,"\n\
is_nan(x) - Return True if x is a qNaN or sNaN, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_normal,"\n\
is_normal(x) - Return True if x is a normal number, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_qnan,"\n\
is_qnan(x) - Return True if x is a quiet NaN, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_signed,"\n\
is_signed(x) - Return True if x is negative, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_snan,"\n\
is_snan() - Return True if x is a signaling NaN, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_subnormal,"\n\
is_subnormal(x) - Return True if x is subnormal, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_zero,"\n\
is_zero(x) - Return True if x is a zero, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_ln,"\n\
ln(x) - Return the natural (base e) logarithm of x.\n\
\n");

PyDoc_STRVAR(doc_ctx_log10,"\n\
log10(x) - Return the base 10 logarithm of x.\n\
\n");

PyDoc_STRVAR(doc_ctx_logb,"\n\
logb(x) - Return the exponent of the magnitude of the operand's MSD.\n\
\n");

PyDoc_STRVAR(doc_ctx_logical_and,"\n\
logical_and(x, y) - Digit-wise and of x and y.\n\
\n");

PyDoc_STRVAR(doc_ctx_logical_invert,"\n\
logical_invert(x) - Invert all digits of x.\n\
\n");

PyDoc_STRVAR(doc_ctx_logical_or,"\n\
logical_or(x, y) - Digit-wise or of x and y.\n\
\n");

PyDoc_STRVAR(doc_ctx_logical_xor,"\n\
logical_xor(x, y) - Digit-wise xor of x and y.\n\
\n");

PyDoc_STRVAR(doc_ctx_max,"\n\
max(x, y) - Compare the values numerically and return the maximum.\n\
\n");

PyDoc_STRVAR(doc_ctx_max_mag,"\n\
max_mag(x, y) - Compare the values numerically with their sign ignored.\n\
\n");

PyDoc_STRVAR(doc_ctx_min,"\n\
min(x, y) - Compare the values numerically and return the minimum.\n\
\n");

PyDoc_STRVAR(doc_ctx_min_mag,"\n\
min_mag(x, y) - Compare the values numerically with their sign ignored.\n\
\n");

PyDoc_STRVAR(doc_ctx_minus,"\n\
minus(x) - Minus corresponds to the unary prefix minus operator in Python,\n\
but applies the context to the result.\n\
\n");

PyDoc_STRVAR(doc_ctx_multiply,"\n\
multiply(x, y) - Return the product of x and y.\n\
\n");

PyDoc_STRVAR(doc_ctx_next_minus,"\n\
next_minus(x) - Return the largest representable number smaller than x.\n\
\n");

PyDoc_STRVAR(doc_ctx_next_plus,"\n\
next_plus(x) - Return the smallest representable number larger than x.\n\
\n");

PyDoc_STRVAR(doc_ctx_next_toward,"\n\
next_toward(x) - Return the number closest to x, in the direction towards y.\n\
\n");

PyDoc_STRVAR(doc_ctx_normalize,"\n\
normalize(x) - Reduce x to its simplest form. Alias for reduce(x).\n\
\n");

PyDoc_STRVAR(doc_ctx_number_class,"\n\
number_class(x) - Return an indication of the class of x.\n\
\n");

PyDoc_STRVAR(doc_ctx_plus,"\n\
plus(x) - Plus corresponds to the unary prefix plus operator in Python,\n\
but applies the context to the result.\n\
\n");

PyDoc_STRVAR(doc_ctx_power,"\n\
power(x, y) - Compute x**y. If x is negative, then y must be integral.\n\
The result will be inexact unless y is integral and the result is finite\n\
and can be expressed exactly in 'precision' digits. In the Python version\n\
the result is always correctly rounded, in the C version the result is\n\
almost always correctly rounded.\n\
\n\
power(x, y, m) - Compute (x**y) % m. The following restrictions hold:\n\
\n\
    * all three arguments must be integral\n\
    * y must be nonnegative\n\
    * at least one of x or y must be nonzero\n\
    * m must be nonzero and less than 10**prec in absolute value\n\
\n\
\n");

PyDoc_STRVAR(doc_ctx_quantize,"\n\
quantize(x, y) - Return a value equal to x (rounded), having the exponent of y.\n\
\n");

PyDoc_STRVAR(doc_ctx_radix,"\n\
radix() - Return 10.\n\
\n");

PyDoc_STRVAR(doc_ctx_remainder,"\n\
remainder(x, y) - Return the remainder from integer division. The sign of\n\
the result, if non-zero, is the same as that of the original dividend.\n\
\n");

PyDoc_STRVAR(doc_ctx_remainder_near,"\n\
remainder_near(x, y) - Return x - y * n, where n is the integer nearest the\n\
exact value of x / y (if the result is 0 then its sign will be the sign of x).\n\
\n");

PyDoc_STRVAR(doc_ctx_rotate,"\n\
rotate(x, y) - Return a copy of x, rotated by y places.\n\
\n");

PyDoc_STRVAR(doc_ctx_same_quantum,"\n\
same_quantum(x, y) - Return True if the two operands have the same exponent.\n\
\n");

PyDoc_STRVAR(doc_ctx_scaleb,"\n\
scaleb(x, y) - Return the first operand after adding the second value\n\
to its exp.\n\
\n");

PyDoc_STRVAR(doc_ctx_shift,"\n\
shift(x, y) - Return a copy of x, shifted by y places.\n\
\n");

PyDoc_STRVAR(doc_ctx_sqrt,"\n\
sqrt(x) - Square root of a non-negative number to context precision.\n\
\n");

PyDoc_STRVAR(doc_ctx_subtract,"\n\
subtract(x, y) - Return the difference between x and y.\n\
\n");

PyDoc_STRVAR(doc_ctx_to_eng_string,"\n\
to_eng_string(x) - Convert a number to a string, using engineering notation.\n\
\n");

PyDoc_STRVAR(doc_ctx_to_integral,"\n\
to_integral(x) - Identical to to_integral_value(x).\n\
\n");

PyDoc_STRVAR(doc_ctx_to_integral_exact,"\n\
to_integral_exact(x) - Round to an integer. Signal if the result is\n\
rounded or inexact.\n\
\n");

PyDoc_STRVAR(doc_ctx_to_integral_value,"\n\
to_integral_value(x) - Round to an integer.\n\
\n");

PyDoc_STRVAR(doc_ctx_to_sci_string,"\n\
to_sci_string(x) - Convert a number to a string using scientific notation.\n\
\n");


#endif /* DOCSTRINGS_H */



