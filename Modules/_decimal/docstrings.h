/*
 * Copyright (c) 2001 Python Software Foundation. All Rights Reserved.
 * Modified and extended by Stefan Krah.
 */


#ifndef DOCSTRINGS_H
#define DOCSTRINGS_H


#include "pymacro.h"

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

PyDoc_STRVAR(doc_is_zero,
"is_zero($self, /)\n--\n\n\
Return True if the argument is a (positive or negative) zero and False\n\
otherwise.\n\
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

PyDoc_STRVAR(doc_ctx_quantize,
"quantize($self, x, y, /)\n--\n\n\
Return a value equal to x (rounded), having the exponent of y.\n\
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



