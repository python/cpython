/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Parameters of the long integer representation.
   These shouldn't have to be changed as C should guarantee that a short
   contains at least 16 bits, but it's made changeable any way.
   Note: 'digit' should be able to hold 2*MASK+1, and 'twodigits'
   should be able to hold the intermediate results in 'mul'
   (at most MASK << SHIFT).
   Also, x_sub assumes that 'digit' is an unsigned type, and overflow
   is handled by taking the result mod 2**N for some N > SHIFT.
   And, at some places it is assumed that MASK fits in an int, as well. */

typedef unsigned short digit;
typedef unsigned long twodigits;

#define SHIFT	15
#define BASE	((digit)1 << SHIFT)
#define MASK	((int)(BASE - 1))

/* Long integer representation.
   The absolute value of a number is equal to
   	SUM(for i=0 through abs(ob_size)-1) ob_digit[i] * 2**(SHIFT*i)
   Negative numbers are represented with ob_size < 0;
   zero is represented by ob_size == 0.
   In a normalized number, ob_digit[abs(ob_size)-1] (the most significant
   digit) is never zero.  Also, in all cases, for all valid i,
   	0 <= ob_digit[i] <= MASK.
   The allocation fuction takes care of allocating extra memory
   so that ob_digit[0] ... ob_digit[abs(ob_size)-1] are actually available. */

typedef struct {
	OB_HEAD
	int ob_size; /* XXX Hack! newvarobj() stores it as unsigned! */
	digit ob_digit[1];
} longobject;

#define ABS(x) ((x) < 0 ? -(x) : (x))

/* Internal use only */
longobject *alloclongobject PROTO((int));
longobject *long_normalize PROTO((longobject *));
longobject *mul1 PROTO((longobject *, digit));
longobject *muladd1 PROTO((longobject *, digit, digit));
longobject *divrem1 PROTO((longobject *, digit, digit *));

/* Check for interrupts during operations on long ints >= this size */
#define INTRLIMIT	20
