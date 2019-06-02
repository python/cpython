/* imath module -- integer-related mathematical functions */

#include "Python.h"

#include "clinic/imathmodule.c.h"

/*[clinic input]
module imath
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=0b5ec353335010dd]*/


/*[clinic input]
imath.as_integer_ratio

    x: object
    /

greatest common divisor of x and y
[clinic start generated code]*/

static PyObject *
imath_as_integer_ratio(PyObject *module, PyObject *x)
/*[clinic end generated code: output=cd8105e5f41e2c52 input=4c6839316976ed71]*/
{
    _Py_IDENTIFIER(as_integer_ratio);
    _Py_IDENTIFIER(numerator);
    _Py_IDENTIFIER(denominator);
    PyObject *ratio, *as_integer_ratio, *numerator, *denominator;

    if (PyLong_CheckExact(x)) {
        return PyTuple_Pack(2, x, _PyLong_One);
    }

    if (_PyObject_LookupAttrId(x, &PyId_as_integer_ratio, &as_integer_ratio) < 0) {
        return NULL;
    }
    if (as_integer_ratio) {
        ratio = _PyObject_CallNoArg(as_integer_ratio);
        Py_DECREF(as_integer_ratio);
        if (ratio == NULL) {
            return NULL;
        }
        if (!PyTuple_Check(ratio)) {
            PyErr_Format(PyExc_TypeError,
                        "unexpected return type from as_integer_ratio(): "
                        "expected tuple, got '%.200s'",
                        Py_TYPE(ratio)->tp_name);
            Py_DECREF(ratio);
            return NULL;
        }
        if (PyTuple_GET_SIZE(ratio) != 2) {
            PyErr_SetString(PyExc_ValueError,
                            "as_integer_ratio() must return a 2-tuple");
            Py_DECREF(ratio);
            return NULL;
        }
    }
    else {
        if (_PyObject_LookupAttrId(x, &PyId_numerator, &numerator) < 0) {
            return NULL;
        }
        if (numerator == NULL) {
            PyErr_Format(PyExc_TypeError,
                         "required a number, not '%.200s'",
                         Py_TYPE(x)->tp_name);
            return NULL;
        }
        if (_PyObject_LookupAttrId(x, &PyId_denominator, &denominator) < 0) {
            Py_DECREF(numerator);
            return NULL;
        }
        if (denominator == NULL) {
            Py_DECREF(numerator);
            PyErr_Format(PyExc_TypeError,
                         "required a number, not '%.200s'",
                         Py_TYPE(x)->tp_name);
            return NULL;
        }
        ratio = PyTuple_Pack(2, numerator, denominator);
        Py_DECREF(numerator);
        Py_DECREF(denominator);
    }
    return ratio;
}


/*[clinic input]
imath.gcd

    x: object
    y: object
    /

Greatest common divisor of x and y.
[clinic start generated code]*/

static PyObject *
imath_gcd_impl(PyObject *module, PyObject *x, PyObject *y)
/*[clinic end generated code: output=14eee3e4a3bd7a1d input=11612898ad79c57c]*/
{
    PyObject *result;

    x = PyNumber_Index(x);
    if (x == NULL)
        return NULL;
    y = PyNumber_Index(y);
    if (y == NULL) {
        Py_DECREF(x);
        return NULL;
    }
    result = _PyLong_GCD(x, y);
    Py_DECREF(x);
    Py_DECREF(y);
    return result;
}


/*[clinic input]
imath.ilog2

    n: object
    /

Return the integer part of the base 2 logarithm of the input.
[clinic start generated code]*/

static PyObject *
imath_ilog2(PyObject *module, PyObject *n)
/*[clinic end generated code: output=6ab48d1a7f5160c2 input=e2d8e8631ec5c29b]*/
{
    size_t bits;

    n = PyNumber_Index(n);
    if (n == NULL) {
        return NULL;
    }

    if (Py_SIZE(n) <= 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "ilog() argument must be positive");
        Py_DECREF(n);
        return NULL;
    }

    bits = _PyLong_NumBits(n);
    Py_DECREF(n);
    if (bits == (size_t)(-1)) {
        return NULL;
    }
    return PyLong_FromSize_t(bits - 1);
}


/* Integer square root

Given a nonnegative integer `n`, we want to compute the largest integer
`a` for which `a * a <= n`, or equivalently the integer part of the exact
square root of `n`.

We use an adaptive-precision pure-integer version of Newton's iteration. Given
a positive integer `n`, the algorithm produces at each iteration an integer
approximation `a` to the square root of `n >> s` for some even integer `s`,
with `s` decreasing as the iterations progress. On the final iteration, `s` is
zero and we have an approximation to the square root of `n` itself.

At every step, the approximation `a` is strictly within 1.0 of the true square
root, so we have

    (a - 1)**2 < (n >> s) < (a + 1)**2

After the final iteration, a check-and-correct step is needed to determine
whether `a` or `a - 1` gives the desired integer square root of `n`.

The algorithm is remarkable in its simplicity. There's no need for a
per-iteration check-and-correct step, and termination is straightforward: the
number of iterations is known in advance (it's exactly `floor(log2(log2(n)))`
for `n > 1`). The only tricky part of the correctness proof is in establishing
that the bound `(a - 1)**2 < (n >> s) < (a + 1)**2` is maintained from one
iteration to the next. A sketch of the proof of this is given below.

In addition to the proof sketch, a formal, computer-verified proof
of correctness (using Lean) of an equivalent recursive algorithm can be found
here:

    https://github.com/mdickinson/snippets/blob/master/proofs/isqrt/src/isqrt.lean


Here's Python code equivalent to the C implementation below:

    def isqrt(n):
        """
        Return the integer part of the square root of the input.
        """
        n = operator.index(n)

        if n < 0:
            raise ValueError("isqrt() argument must be nonnegative")
        if n == 0:
            return 0

        c = (n.bit_length() - 1) // 2
        a = 1
        d = 0
        for s in reversed(range(c.bit_length())):
            e = d
            d = c >> s
            a = (a << d - e - 1) + (n >> 2*c - e - d + 1) // a
            assert (a-1)**2 < n >> 2*(c - d) < (a+1)**2

        return a - (a*a > n)


Sketch of proof of correctness
------------------------------

The delicate part of the correctness proof is showing that the loop invariant
is preserved from one iteration to the next. That is, just before the line

    a = (a << d - e - 1) + (n >> 2*c - e - d + 1) // a

is executed in the above code, we know that

    (1)  (a - 1)**2 < (n >> 2*(c - e)) < (a + 1)**2.

(since `e` is always the value of `d` from the previous iteration). We must
prove that after that line is executed, we have

    (a - 1)**2 < (n >> 2*(c - d)) < (a + 1)**2

To faciliate the proof, we make some changes of notation. Write `m` for
`n >> 2*(c-d)`, and write `b` for the new value of `a`, so

    b = (a << d - e - 1) + (n >> 2*c - e - d + 1) // a

or equivalently:

    (2)  b = (a << d - e - 1) + (m >> d - e + 1) // a

Then we can rewrite (1) as:

    (3)  (a - 1)**2 < (m >> 2*(d - e)) < (a + 1)**2

and we must show that (b - 1)**2 < m < (b + 1)**2.

From this point on, we switch to mathematical notation, so `/` means exact
division rather than integer division and `^` is used for exponentiation. We
use the `√` symbol for the exact square root. In (3), we can remove the
implicit floor operation to give:

    (4)  (a - 1)^2 < m / 4^(d - e) < (a + 1)^2

Taking square roots throughout (4), scaling by `2^(d-e)`, and rearranging gives

    (5)  0 <= | 2^(d-e)a - √m | < 2^(d-e)

Squaring and dividing through by `2^(d-e+1) a` gives

    (6)  0 <= 2^(d-e-1) a + m / (2^(d-e+1) a) - √m < 2^(d-e-1) / a

We'll show below that `2^(d-e-1) <= a`. Given that, we can replace the
right-hand side of (6) with `1`, and now replacing the central
term `m / (2^(d-e+1) a)` with its floor in (6) gives

    (7) -1 < 2^(d-e-1) a + m // 2^(d-e+1) a - √m < 1

Or equivalently, from (2):

    (7) -1 < b - √m < 1

and rearranging gives that `(b-1)^2 < m < (b+1)^2`, which is what we needed
to prove.

We're not quite done: we still have to prove the inequality `2^(d - e - 1) <=
a` that was used to get line (7) above. From the definition of `c`, we have
`4^c <= n`, which implies

    (8)  4^d <= m

also, since `e == d >> 1`, `d` is at most `2e + 1`, from which it follows
that `2d - 2e - 1 <= d` and hence that

    (9)  4^(2d - 2e - 1) <= m

Dividing both sides by `4^(d - e)` gives

    (10)  4^(d - e - 1) <= m / 4^(d - e)

But we know from (4) that `m / 4^(d-e) < (a + 1)^2`, hence

    (11)  4^(d - e - 1) < (a + 1)^2

Now taking square roots of both sides and observing that both `2^(d-e-1)` and
`a` are integers gives `2^(d - e - 1) <= a`, which is what we needed. This
completes the proof sketch.

*/


/* Approximate square root of a large 64-bit integer.

   Given `n` satisfying `2**62 <= n < 2**64`, return `a`
   satisfying `(a - 1)**2 < n < (a + 1)**2`. */

static uint64_t
_approximate_isqrt(uint64_t n)
{
    uint32_t u = 1U + (n >> 62);
    u = (u << 1) + (n >> 59) / u;
    u = (u << 3) + (n >> 53) / u;
    u = (u << 7) + (n >> 41) / u;
    return (u << 15) + (n >> 17) / u;
}

/*[clinic input]
imath.isqrt

    n: object
    /

Return the integer part of the square root of the input.
[clinic start generated code]*/

static PyObject *
imath_isqrt(PyObject *module, PyObject *n)
/*[clinic end generated code: output=5ad16a80dd47c888 input=64f5b3e586986fc9]*/
{
    int a_too_large, c_bit_length;
    size_t c, d;
    uint64_t m, u;
    PyObject *a = NULL, *b;

    n = PyNumber_Index(n);
    if (n == NULL) {
        return NULL;
    }

    if (_PyLong_Sign(n) < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "isqrt() argument must be nonnegative");
        goto error;
    }
    if (_PyLong_Sign(n) == 0) {
        Py_DECREF(n);
        return PyLong_FromLong(0);
    }

    /* c = (n.bit_length() - 1) // 2 */
    c = _PyLong_NumBits(n);
    if (c == (size_t)(-1)) {
        goto error;
    }
    c = (c - 1U) / 2U;

    /* Fast path: if c <= 31 then n < 2**64 and we can compute directly with a
       fast, almost branch-free algorithm. In the final correction, we use `u*u
       - 1 >= m` instead of the simpler `u*u > m` in order to get the correct
       result in the corner case where `u=2**32`. */
    if (c <= 31U) {
        m = (uint64_t)PyLong_AsUnsignedLongLong(n);
        Py_DECREF(n);
        if (m == (uint64_t)(-1) && PyErr_Occurred()) {
            return NULL;
        }
        u = _approximate_isqrt(m << (62U - 2U*c)) >> (31U - c);
        u -= u * u - 1U >= m;
        return PyLong_FromUnsignedLongLong((unsigned long long)u);
    }

    /* Slow path: n >= 2**64. We perform the first five iterations in C integer
       arithmetic, then switch to using Python long integers. */

    /* From n >= 2**64 it follows that c.bit_length() >= 6. */
    c_bit_length = 6;
    while ((c >> c_bit_length) > 0U) {
        ++c_bit_length;
    }

    /* Initialise d and a. */
    d = c >> (c_bit_length - 5);
    b = _PyLong_Rshift(n, 2U*c - 62U);
    if (b == NULL) {
        goto error;
    }
    m = (uint64_t)PyLong_AsUnsignedLongLong(b);
    Py_DECREF(b);
    if (m == (uint64_t)(-1) && PyErr_Occurred()) {
        goto error;
    }
    u = _approximate_isqrt(m) >> (31U - d);
    a = PyLong_FromUnsignedLongLong((unsigned long long)u);
    if (a == NULL) {
        goto error;
    }

    for (int s = c_bit_length - 6; s >= 0; --s) {
        PyObject *q;
        size_t e = d;

        d = c >> s;

        /* q = (n >> 2*c - e - d + 1) // a */
        q = _PyLong_Rshift(n, 2U*c - d - e + 1U);
        if (q == NULL) {
            goto error;
        }
        Py_SETREF(q, PyNumber_FloorDivide(q, a));
        if (q == NULL) {
            goto error;
        }

        /* a = (a << d - 1 - e) + q */
        Py_SETREF(a, _PyLong_Lshift(a, d - 1U - e));
        if (a == NULL) {
            Py_DECREF(q);
            goto error;
        }
        Py_SETREF(a, PyNumber_Add(a, q));
        Py_DECREF(q);
        if (a == NULL) {
            goto error;
        }
    }

    /* The correct result is either a or a - 1. Figure out which, and
       decrement a if necessary. */

    /* a_too_large = n < a * a */
    b = PyNumber_Multiply(a, a);
    if (b == NULL) {
        goto error;
    }
    a_too_large = PyObject_RichCompareBool(n, b, Py_LT);
    Py_DECREF(b);
    if (a_too_large == -1) {
        goto error;
    }

    if (a_too_large) {
        Py_SETREF(a, PyNumber_Subtract(a, _PyLong_One));
    }
    Py_DECREF(n);
    return a;

  error:
    Py_XDECREF(a);
    Py_DECREF(n);
    return NULL;
}


/* Return the smallest integer k such that n < 2**k, or 0 if n == 0.
 * Equivalent to floor(lg(x))+1.  Also equivalent to: bitwidth_of_type -
 * count_leading_zero_bits(x)
 */

/* XXX: This routine does more or less the same thing as
 * bits_in_digit() in Objects/longobject.c.  Someday it would be nice to
 * consolidate them.  On BSD, there's a library function called fls()
 * that we could use, and GCC provides __builtin_clz().
 */

static unsigned long
bit_length(unsigned long n)
{
    unsigned long len = 0;
    while (n != 0) {
        ++len;
        n >>= 1;
    }
    return len;
}

static unsigned long
count_set_bits(unsigned long n)
{
    unsigned long count = 0;
    while (n != 0) {
        ++count;
        n &= n - 1; /* clear least significant bit */
    }
    return count;
}


/* Divide-and-conquer factorial algorithm
 *
 * Based on the formula and pseudo-code provided at:
 * http://www.luschny.de/math/factorial/binarysplitfact.html
 *
 * Faster algorithms exist, but they're more complicated and depend on
 * a fast prime factorization algorithm.
 *
 * Notes on the algorithm
 * ----------------------
 *
 * factorial(n) is written in the form 2**k * m, with m odd.  k and m are
 * computed separately, and then combined using a left shift.
 *
 * The function factorial_odd_part computes the odd part m (i.e., the greatest
 * odd divisor) of factorial(n), using the formula:
 *
 *   factorial_odd_part(n) =
 *
 *        product_{i >= 0} product_{0 < j <= n / 2**i, j odd} j
 *
 * Example: factorial_odd_part(20) =
 *
 *        (1) *
 *        (1) *
 *        (1 * 3 * 5) *
 *        (1 * 3 * 5 * 7 * 9)
 *        (1 * 3 * 5 * 7 * 9 * 11 * 13 * 15 * 17 * 19)
 *
 * Here i goes from large to small: the first term corresponds to i=4 (any
 * larger i gives an empty product), and the last term corresponds to i=0.
 * Each term can be computed from the last by multiplying by the extra odd
 * numbers required: e.g., to get from the penultimate term to the last one,
 * we multiply by (11 * 13 * 15 * 17 * 19).
 *
 * To see a hint of why this formula works, here are the same numbers as above
 * but with the even parts (i.e., the appropriate powers of 2) included.  For
 * each subterm in the product for i, we multiply that subterm by 2**i:
 *
 *   factorial(20) =
 *
 *        (16) *
 *        (8) *
 *        (4 * 12 * 20) *
 *        (2 * 6 * 10 * 14 * 18) *
 *        (1 * 3 * 5 * 7 * 9 * 11 * 13 * 15 * 17 * 19)
 *
 * The factorial_partial_product function computes the product of all odd j in
 * range(start, stop) for given start and stop.  It's used to compute the
 * partial products like (11 * 13 * 15 * 17 * 19) in the example above.  It
 * operates recursively, repeatedly splitting the range into two roughly equal
 * pieces until the subranges are small enough to be computed using only C
 * integer arithmetic.
 *
 * The two-valuation k (i.e., the exponent of the largest power of 2 dividing
 * the factorial) is computed independently in the main math_factorial
 * function.  By standard results, its value is:
 *
 *    two_valuation = n//2 + n//4 + n//8 + ....
 *
 * It can be shown (e.g., by complete induction on n) that two_valuation is
 * equal to n - count_set_bits(n), where count_set_bits(n) gives the number of
 * '1'-bits in the binary expansion of n.
 */

/* factorial_partial_product: Compute product(range(start, stop, 2)) using
 * divide and conquer.  Assumes start and stop are odd and stop > start.
 * max_bits must be >= bit_length(stop - 2). */

static PyObject *
factorial_partial_product(unsigned long start, unsigned long stop,
                          unsigned long max_bits)
{
    unsigned long midpoint, num_operands;
    PyObject *left = NULL, *right = NULL, *result = NULL;

    /* If the return value will fit an unsigned long, then we can
     * multiply in a tight, fast loop where each multiply is O(1).
     * Compute an upper bound on the number of bits required to store
     * the answer.
     *
     * Storing some integer z requires floor(lg(z))+1 bits, which is
     * conveniently the value returned by bit_length(z).  The
     * product x*y will require at most
     * bit_length(x) + bit_length(y) bits to store, based
     * on the idea that lg product = lg x + lg y.
     *
     * We know that stop - 2 is the largest number to be multiplied.  From
     * there, we have: bit_length(answer) <= num_operands *
     * bit_length(stop - 2)
     */

    num_operands = (stop - start) / 2;
    /* The "num_operands <= 8 * SIZEOF_LONG" check guards against the
     * unlikely case of an overflow in num_operands * max_bits. */
    if (num_operands <= 8 * SIZEOF_LONG &&
        num_operands * max_bits <= 8 * SIZEOF_LONG) {
        unsigned long j, total;
        for (total = start, j = start + 2; j < stop; j += 2)
            total *= j;
        return PyLong_FromUnsignedLong(total);
    }

    /* find midpoint of range(start, stop), rounded up to next odd number. */
    midpoint = (start + num_operands) | 1;
    left = factorial_partial_product(start, midpoint,
                                     bit_length(midpoint - 2));
    if (left == NULL)
        goto error;
    right = factorial_partial_product(midpoint, stop, max_bits);
    if (right == NULL)
        goto error;
    result = PyNumber_Multiply(left, right);

  error:
    Py_XDECREF(left);
    Py_XDECREF(right);
    return result;
}

/* factorial_odd_part:  compute the odd part of factorial(n). */

static PyObject *
factorial_odd_part(unsigned long n)
{
    long i;
    unsigned long v, lower, upper;
    PyObject *partial, *tmp, *inner, *outer;

    inner = PyLong_FromLong(1);
    if (inner == NULL)
        return NULL;
    outer = inner;
    Py_INCREF(outer);

    upper = 3;
    for (i = bit_length(n) - 2; i >= 0; i--) {
        v = n >> i;
        if (v <= 2)
            continue;
        lower = upper;
        /* (v + 1) | 1 = least odd integer strictly larger than n / 2**i */
        upper = (v + 1) | 1;
        /* Here inner is the product of all odd integers j in the range (0,
           n/2**(i+1)].  The factorial_partial_product call below gives the
           product of all odd integers j in the range (n/2**(i+1), n/2**i]. */
        partial = factorial_partial_product(lower, upper, bit_length(upper-2));
        /* inner *= partial */
        if (partial == NULL)
            goto error;
        tmp = PyNumber_Multiply(inner, partial);
        Py_DECREF(partial);
        if (tmp == NULL)
            goto error;
        Py_DECREF(inner);
        inner = tmp;
        /* Now inner is the product of all odd integers j in the range (0,
           n/2**i], giving the inner product in the formula above. */

        /* outer *= inner; */
        tmp = PyNumber_Multiply(outer, inner);
        if (tmp == NULL)
            goto error;
        Py_DECREF(outer);
        outer = tmp;
    }
    Py_DECREF(inner);
    return outer;

  error:
    Py_DECREF(outer);
    Py_DECREF(inner);
    return NULL;
}


/* Lookup table for small factorial values */

static const unsigned long SmallFactorials[] = {
    1, 1, 2, 6, 24, 120, 720, 5040, 40320,
    362880, 3628800, 39916800, 479001600,
#if SIZEOF_LONG >= 8
    6227020800, 87178291200, 1307674368000,
    20922789888000, 355687428096000, 6402373705728000,
    121645100408832000, 2432902008176640000
#endif
};

PyObject *_PyLong_Factorial(PyObject *arg);

/*[clinic input]
imath.factorial

    x as arg: object
    /

Find x!.

Raise a TypeError if x is not an integer.
Raise a ValueError if x is negative integer.
[clinic start generated code]*/

static PyObject *
imath_factorial(PyObject *module, PyObject *arg)
/*[clinic end generated code: output=73f1879dcbd64aea input=d5f41d496efcaf51]*/
{
    return _PyLong_Factorial(arg);
}

PyObject *
_PyLong_Factorial(PyObject *arg)
{
    long x, two_valuation;
    int overflow;
    PyObject *result, *odd_part, *pyint_form;

    pyint_form = PyNumber_Index(arg);
    if (pyint_form == NULL) {
        return NULL;
    }
    x = PyLong_AsLongAndOverflow(pyint_form, &overflow);
    Py_DECREF(pyint_form);
    if (x == -1 && PyErr_Occurred()) {
        return NULL;
    }
    else if (overflow == 1) {
        PyErr_Format(PyExc_OverflowError,
                     "factorial() argument should not exceed %ld",
                     LONG_MAX);
        return NULL;
    }
    else if (overflow == -1 || x < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "factorial() not defined for negative values");
        return NULL;
    }

    /* use lookup table if x is small */
    if (x < (long)Py_ARRAY_LENGTH(SmallFactorials))
        return PyLong_FromUnsignedLong(SmallFactorials[x]);

    /* else express in the form odd_part * 2**two_valuation, and compute as
       odd_part << two_valuation. */
    odd_part = factorial_odd_part(x);
    if (odd_part == NULL)
        return NULL;
    two_valuation = x - count_set_bits(x);
    result = _PyLong_Lshift(odd_part, two_valuation);
    Py_DECREF(odd_part);
    return result;
}


/*[clinic input]
imath.perm

    n: object
    k: object
    /

Number of ways to choose k items from n items without repetition and with order.

It is mathematically equal to the expression n! / (n - k)!.

Raises TypeError if the arguments are not integers.
Raises ValueError if the arguments are negative or if k > n.
[clinic start generated code]*/

static PyObject *
imath_perm_impl(PyObject *module, PyObject *n, PyObject *k)
/*[clinic end generated code: output=4b4f3aa22c47911e input=24aa606dab86900c]*/
{
    PyObject *result = NULL, *factor = NULL;
    int overflow, cmp;
    long long i, factors;

    n = PyNumber_Index(n);
    if (n == NULL) {
        return NULL;
    }
    if (!PyLong_CheckExact(n)) {
        Py_SETREF(n, _PyLong_Copy((PyLongObject *)n));
        if (n == NULL) {
            return NULL;
        }
    }
    k = PyNumber_Index(k);
    if (k == NULL) {
        Py_DECREF(n);
        return NULL;
    }
    if (!PyLong_CheckExact(k)) {
        Py_SETREF(k, _PyLong_Copy((PyLongObject *)k));
        if (k == NULL) {
            Py_DECREF(n);
            return NULL;
        }
    }

    if (Py_SIZE(n) < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "n must be a non-negative integer");
        goto error;
    }
    cmp = PyObject_RichCompareBool(n, k, Py_LT);
    if (cmp != 0) {
        if (cmp > 0) {
            PyErr_SetString(PyExc_ValueError,
                            "k must be an integer less than or equal to n");
        }
        goto error;
    }

    factors = PyLong_AsLongLongAndOverflow(k, &overflow);
    if (overflow > 0) {
        PyErr_Format(PyExc_OverflowError,
                     "k must not exceed %lld",
                     LLONG_MAX);
        goto error;
    }
    else if (overflow < 0 || factors < 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError,
                            "k must be a non-negative integer");
        }
        goto error;
    }

    if (factors == 0) {
        result = PyLong_FromLong(1);
        goto done;
    }

    result = n;
    Py_INCREF(result);
    if (factors == 1) {
        goto done;
    }

    factor = n;
    Py_INCREF(factor);
    for (i = 1; i < factors; ++i) {
        Py_SETREF(factor, PyNumber_Subtract(factor, _PyLong_One));
        if (factor == NULL) {
            goto error;
        }
        Py_SETREF(result, PyNumber_Multiply(result, factor));
        if (result == NULL) {
            goto error;
        }
    }
    Py_DECREF(factor);

done:
    Py_DECREF(n);
    Py_DECREF(k);
    return result;

error:
    Py_XDECREF(factor);
    Py_XDECREF(result);
    Py_DECREF(n);
    Py_DECREF(k);
    return NULL;
}


/*[clinic input]
imath.comb

    n: object
    k: object
    /

Number of ways to choose k items from n items without repetition and without order.

Also called the binomial coefficient. It is mathematically equal to the expression
n! / (k! * (n - k)!). It is equivalent to the coefficient of k-th term in
polynomial expansion of the expression (1 + x)**n.

Raises TypeError if the arguments are not integers.
Raises ValueError if the arguments are negative or if k > n.

[clinic start generated code]*/

static PyObject *
imath_comb_impl(PyObject *module, PyObject *n, PyObject *k)
/*[clinic end generated code: output=87cda746ba0145ef input=f6376b6622fdc123]*/
{
    PyObject *result = NULL, *factor = NULL, *temp;
    int overflow, cmp;
    long long i, factors;

    n = PyNumber_Index(n);
    if (n == NULL) {
        return NULL;
    }
    if (!PyLong_CheckExact(n)) {
        Py_SETREF(n, _PyLong_Copy((PyLongObject *)n));
        if (n == NULL) {
            return NULL;
        }
    }
    k = PyNumber_Index(k);
    if (k == NULL) {
        Py_DECREF(n);
        return NULL;
    }
    if (!PyLong_CheckExact(k)) {
        Py_SETREF(k, _PyLong_Copy((PyLongObject *)k));
        if (k == NULL) {
            Py_DECREF(n);
            return NULL;
        }
    }

    if (Py_SIZE(n) < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "n must be a non-negative integer");
        goto error;
    }
    /* k = min(k, n - k) */
    temp = PyNumber_Subtract(n, k);
    if (temp == NULL) {
        goto error;
    }
    if (Py_SIZE(temp) < 0) {
        Py_DECREF(temp);
        PyErr_SetString(PyExc_ValueError,
                        "k must be an integer less than or equal to n");
        goto error;
    }
    cmp = PyObject_RichCompareBool(temp, k, Py_LT);
    if (cmp > 0) {
        Py_SETREF(k, temp);
    }
    else {
        Py_DECREF(temp);
        if (cmp < 0) {
            goto error;
        }
    }

    factors = PyLong_AsLongLongAndOverflow(k, &overflow);
    if (overflow > 0) {
        PyErr_Format(PyExc_OverflowError,
                     "min(n - k, k) must not exceed %lld",
                     LLONG_MAX);
        goto error;
    }
    else if (overflow < 0 || factors < 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError,
                            "k must be a non-negative integer");
        }
        goto error;
    }

    if (factors == 0) {
        result = PyLong_FromLong(1);
        goto done;
    }

    result = n;
    Py_INCREF(result);
    if (factors == 1) {
        goto done;
    }

    factor = n;
    Py_INCREF(factor);
    for (i = 1; i < factors; ++i) {
        Py_SETREF(factor, PyNumber_Subtract(factor, _PyLong_One));
        if (factor == NULL) {
            goto error;
        }
        Py_SETREF(result, PyNumber_Multiply(result, factor));
        if (result == NULL) {
            goto error;
        }

        temp = PyLong_FromUnsignedLongLong((unsigned long long)i + 1);
        if (temp == NULL) {
            goto error;
        }
        Py_SETREF(result, PyNumber_FloorDivide(result, temp));
        Py_DECREF(temp);
        if (result == NULL) {
            goto error;
        }
    }
    Py_DECREF(factor);

done:
    Py_DECREF(n);
    Py_DECREF(k);
    return result;

error:
    Py_XDECREF(factor);
    Py_XDECREF(result);
    Py_DECREF(n);
    Py_DECREF(k);
    return NULL;
}


static PyMethodDef imath_methods[] = {
    IMATH_AS_INTEGER_RATIO_METHODDEF
    IMATH_COMB_METHODDEF
    IMATH_FACTORIAL_METHODDEF
    IMATH_GCD_METHODDEF
    IMATH_ILOG2_METHODDEF
    IMATH_ISQRT_METHODDEF
    IMATH_PERM_METHODDEF
    {NULL,              NULL}           /* sentinel */
};


PyDoc_STRVAR(module_doc,
"This module provides access to integer related mathematical functions.");


static struct PyModuleDef imathmodule = {
    PyModuleDef_HEAD_INIT,
    "imath",
    module_doc,
    -1,
    imath_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_imath(void)
{
    return PyModule_Create(&imathmodule);
}
