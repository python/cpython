/*
 * Copyright (c) 2001 Python Software Foundation. All Rights Reserved.
 * Modified and extended by Stefan Krah.
 */


#ifndef DOCSTRINGS_H
#define DOCSTRINGS_H


#include "pymacro.h"

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

PyDoc_STRVAR(doc_ctx_canonical,
"canonical($self, x, /)\n--\n\n\
Return a new instance of x.\n\
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

PyDoc_STRVAR(doc_ctx_divmod,
"divmod($self, x, y, /)\n--\n\n\
Return quotient and remainder of the division x / y.\n\
\n");

PyDoc_STRVAR(doc_ctx_is_canonical,
"is_canonical($self, x, /)\n--\n\n\
Return True if x is canonical, False otherwise.\n\
\n");

PyDoc_STRVAR(doc_ctx_normalize,
"normalize($self, x, /)\n--\n\n\
Reduce x to its simplest form. Alias for reduce(x).\n\
\n");

PyDoc_STRVAR(doc_ctx_number_class,
"number_class($self, x, /)\n--\n\n\
Return an indication of the class of x.\n\
\n");

PyDoc_STRVAR(doc_ctx_same_quantum,
"same_quantum($self, x, y, /)\n--\n\n\
Return True if the two operands have the same exponent.\n\
\n");

PyDoc_STRVAR(doc_ctx_to_eng_string,
"to_eng_string($self, x, /)\n--\n\n\
Convert a number to a string, using engineering notation.\n\
\n");

PyDoc_STRVAR(doc_ctx_to_sci_string,
"to_sci_string($self, x, /)\n--\n\n\
Convert a number to a string using scientific notation.\n\
\n");


#endif /* DOCSTRINGS_H */



