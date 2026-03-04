/* math.integer module -- integer-related mathematical functions */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_bitutils.h"      // _Py_bit_length()
#include "pycore_long.h"          // _PyLong_GetZero()

#include "clinic/mathintegermodule.c.h"

/*[clinic input]
module math
module math.integer
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e3d09c1c90de7fa8]*/


/*[clinic input]
math.integer.gcd

    *integers as args: array

Greatest Common Divisor.
[clinic start generated code]*/

static PyObject *
math_integer_gcd_impl(PyObject *module, PyObject * const *args,
                      Py_ssize_t args_length)
/*[clinic end generated code: output=8e9c5bab06bea203 input=a90cde2ac5281551]*/
{
    // Fast-path for the common case: gcd(int, int)
    if (args_length == 2 && PyLong_CheckExact(args[0]) && PyLong_CheckExact(args[1]))
    {
        return _PyLong_GCD(args[0], args[1]);
    }

    if (args_length == 0) {
        return PyLong_FromLong(0);
    }

    PyObject *res = PyNumber_Index(args[0]);
    if (res == NULL) {
        return NULL;
    }
    if (args_length == 1) {
        Py_SETREF(res, PyNumber_Absolute(res));
        return res;
    }

    PyObject *one = _PyLong_GetOne();  // borrowed ref
    for (Py_ssize_t i = 1; i < args_length; i++) {
        PyObject *x = _PyNumber_Index(args[i]);
        if (x == NULL) {
            Py_DECREF(res);
            return NULL;
        }
        if (res == one) {
            /* Fast path: just check arguments.
               It is okay to use identity comparison here. */
            Py_DECREF(x);
            continue;
        }
        Py_SETREF(res, _PyLong_GCD(res, x));
        Py_DECREF(x);
        if (res == NULL) {
            return NULL;
        }
    }
    return res;
}


static PyObject *
long_lcm(PyObject *a, PyObject *b)
{
    PyObject *g, *m, *f, *ab;

    if (_PyLong_IsZero((PyLongObject *)a) || _PyLong_IsZero((PyLongObject *)b)) {
        return PyLong_FromLong(0);
    }
    g = _PyLong_GCD(a, b);
    if (g == NULL) {
        return NULL;
    }
    f = PyNumber_FloorDivide(a, g);
    Py_DECREF(g);
    if (f == NULL) {
        return NULL;
    }
    m = PyNumber_Multiply(f, b);
    Py_DECREF(f);
    if (m == NULL) {
        return NULL;
    }
    ab = PyNumber_Absolute(m);
    Py_DECREF(m);
    return ab;
}


/*[clinic input]
math.integer.lcm

    *integers as args: array

Least Common Multiple.
[clinic start generated code]*/

static PyObject *
math_integer_lcm_impl(PyObject *module, PyObject * const *args,
                      Py_ssize_t args_length)
/*[clinic end generated code: output=3e88889b866ccc28 input=261bddc85a136bdf]*/
{
    PyObject *res, *x;
    Py_ssize_t i;

    if (args_length == 0) {
        return PyLong_FromLong(1);
    }
    res = PyNumber_Index(args[0]);
    if (res == NULL) {
        return NULL;
    }
    if (args_length == 1) {
        Py_SETREF(res, PyNumber_Absolute(res));
        return res;
    }

    PyObject *zero = _PyLong_GetZero();  // borrowed ref
    for (i = 1; i < args_length; i++) {
        x = PyNumber_Index(args[i]);
        if (x == NULL) {
            Py_DECREF(res);
            return NULL;
        }
        if (res == zero) {
            /* Fast path: just check arguments.
               It is okay to use identity comparison here. */
            Py_DECREF(x);
            continue;
        }
        Py_SETREF(res, long_lcm(res, x));
        Py_DECREF(x);
        if (res == NULL) {
            return NULL;
        }
    }
    return res;
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
            # Loop invariant: (a-1)**2 < (n >> 2*(c - d)) < (a+1)**2
            e = d
            d = c >> s
            a = (a << d - e - 1) + (n >> 2*c - e - d + 1) // a

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

To facilitate the proof, we make some changes of notation. Write `m` for
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

/*
    The _approximate_isqrt_tab table provides approximate square roots for
    16-bit integers. For any n in the range 2**14 <= n < 2**16, the value

        a = _approximate_isqrt_tab[(n >> 8) - 64]

    is an approximate square root of n, satisfying (a - 1)**2 < n < (a + 1)**2.

    The table was computed in Python using the expression:

        [min(round(sqrt(256*n + 128)), 255) for n in range(64, 256)]
*/

static const uint8_t _approximate_isqrt_tab[192] = {
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
    140, 141, 142, 143, 144, 144, 145, 146, 147, 148, 149, 150,
    151, 151, 152, 153, 154, 155, 156, 156, 157, 158, 159, 160,
    160, 161, 162, 163, 164, 164, 165, 166, 167, 167, 168, 169,
    170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178,
    179, 179, 180, 181, 181, 182, 183, 183, 184, 185, 186, 186,
    187, 188, 188, 189, 190, 190, 191, 192, 192, 193, 194, 194,
    195, 196, 196, 197, 198, 198, 199, 200, 200, 201, 201, 202,
    203, 203, 204, 205, 205, 206, 206, 207, 208, 208, 209, 210,
    210, 211, 211, 212, 213, 213, 214, 214, 215, 216, 216, 217,
    217, 218, 219, 219, 220, 220, 221, 221, 222, 223, 223, 224,
    224, 225, 225, 226, 227, 227, 228, 228, 229, 229, 230, 230,
    231, 232, 232, 233, 233, 234, 234, 235, 235, 236, 237, 237,
    238, 238, 239, 239, 240, 240, 241, 241, 242, 242, 243, 243,
    244, 244, 245, 246, 246, 247, 247, 248, 248, 249, 249, 250,
    250, 251, 251, 252, 252, 253, 253, 254, 254, 255, 255, 255,
};

/* Approximate square root of a large 64-bit integer.

   Given `n` satisfying `2**62 <= n < 2**64`, return `a`
   satisfying `(a - 1)**2 < n < (a + 1)**2`. */

static inline uint32_t
_approximate_isqrt(uint64_t n)
{
    uint32_t u = _approximate_isqrt_tab[(n >> 56) - 64];
    u = (u << 7) + (uint32_t)(n >> 41) / u;
    return (u << 15) + (uint32_t)((n >> 17) / u);
}

/*[clinic input]
math.integer.isqrt

    n: object
    /

Return the integer part of the square root of the input.
[clinic start generated code]*/

static PyObject *
math_integer_isqrt(PyObject *module, PyObject *n)
/*[clinic end generated code: output=551031e41a0f5d9e input=921ddd9853133d8d]*/
{
    int a_too_large, c_bit_length;
    int64_t c, d;
    uint64_t m;
    uint32_t u;
    PyObject *a = NULL, *b;

    n = _PyNumber_Index(n);
    if (n == NULL) {
        return NULL;
    }

    if (_PyLong_IsNegative((PyLongObject *)n)) {
        PyErr_SetString(
            PyExc_ValueError,
            "isqrt() argument must be nonnegative");
        goto error;
    }
    if (_PyLong_IsZero((PyLongObject *)n)) {
        Py_DECREF(n);
        return PyLong_FromLong(0);
    }

    /* c = (n.bit_length() - 1) // 2 */
    c = _PyLong_NumBits(n);
    assert(c > 0);
    assert(!PyErr_Occurred());
    c = (c - 1) / 2;

    /* Fast path: if c <= 31 then n < 2**64 and we can compute directly with a
       fast, almost branch-free algorithm. */
    if (c <= 31) {
        int shift = 31 - (int)c;
        m = (uint64_t)PyLong_AsUnsignedLongLong(n);
        Py_DECREF(n);
        if (m == (uint64_t)(-1) && PyErr_Occurred()) {
            return NULL;
        }
        u = _approximate_isqrt(m << 2*shift) >> shift;
        u -= (uint64_t)u * u > m;
        return PyLong_FromUnsignedLong(u);
    }

    /* Slow path: n >= 2**64. We perform the first five iterations in C integer
       arithmetic, then switch to using Python long integers. */

    /* From n >= 2**64 it follows that c.bit_length() >= 6. */
    c_bit_length = 6;
    while ((c >> c_bit_length) > 0) {
        ++c_bit_length;
    }

    /* Initialise d and a. */
    d = c >> (c_bit_length - 5);
    b = _PyLong_Rshift(n, 2*c - 62);
    if (b == NULL) {
        goto error;
    }
    m = (uint64_t)PyLong_AsUnsignedLongLong(b);
    Py_DECREF(b);
    if (m == (uint64_t)(-1) && PyErr_Occurred()) {
        goto error;
    }
    u = _approximate_isqrt(m) >> (31U - d);
    a = PyLong_FromUnsignedLong(u);
    if (a == NULL) {
        goto error;
    }

    for (int s = c_bit_length - 6; s >= 0; --s) {
        PyObject *q;
        int64_t e = d;

        d = c >> s;

        /* q = (n >> 2*c - e - d + 1) // a */
        q = _PyLong_Rshift(n, 2*c - d - e + 1);
        if (q == NULL) {
            goto error;
        }
        Py_SETREF(q, PyNumber_FloorDivide(q, a));
        if (q == NULL) {
            goto error;
        }

        /* a = (a << d - 1 - e) + q */
        Py_SETREF(a, _PyLong_Lshift(a, d - 1 - e));
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
        Py_SETREF(a, PyNumber_Subtract(a, _PyLong_GetOne()));
    }
    Py_DECREF(n);
    return a;

  error:
    Py_XDECREF(a);
    Py_DECREF(n);
    return NULL;
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
 *        (1 * 3 * 5 * 7 * 9) *
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
 * the factorial) is computed independently in the main math_integer_factorial
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
                                     _Py_bit_length(midpoint - 2));
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
    outer = Py_NewRef(inner);

    upper = 3;
    for (i = _Py_bit_length(n) - 2; i >= 0; i--) {
        v = n >> i;
        if (v <= 2)
            continue;
        lower = upper;
        /* (v + 1) | 1 = least odd integer strictly larger than n / 2**i */
        upper = (v + 1) | 1;
        /* Here inner is the product of all odd integers j in the range (0,
           n/2**(i+1)].  The factorial_partial_product call below gives the
           product of all odd integers j in the range (n/2**(i+1), n/2**i]. */
        partial = factorial_partial_product(lower, upper, _Py_bit_length(upper-2));
        /* inner *= partial */
        if (partial == NULL)
            goto error;
        tmp = PyNumber_Multiply(inner, partial);
        Py_DECREF(partial);
        if (tmp == NULL)
            goto error;
        Py_SETREF(inner, tmp);
        /* Now inner is the product of all odd integers j in the range (0,
           n/2**i], giving the inner product in the formula above. */

        /* outer *= inner; */
        tmp = PyNumber_Multiply(outer, inner);
        if (tmp == NULL)
            goto error;
        Py_SETREF(outer, tmp);
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

/*[clinic input]
math.integer.factorial

    n as arg: object
    /

Find n!.
[clinic start generated code]*/

static PyObject *
math_integer_factorial(PyObject *module, PyObject *arg)
/*[clinic end generated code: output=131c23fd48650414 input=742f4dfa490a1b07]*/
{
    long x, two_valuation;
    int overflow;
    PyObject *result, *odd_part;

    x = PyLong_AsLongAndOverflow(arg, &overflow);
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


/* least significant 64 bits of the odd part of factorial(n), for n in range(128).

Python code to generate the values:

    import math.integer

    for n in range(128):
        fac = math.integer.factorial(n)
        fac_odd_part = fac // (fac & -fac)
        reduced_fac_odd_part = fac_odd_part % (2**64)
        print(f"{reduced_fac_odd_part:#018x}u")
*/
static const uint64_t reduced_factorial_odd_part[] = {
    0x0000000000000001u, 0x0000000000000001u, 0x0000000000000001u, 0x0000000000000003u,
    0x0000000000000003u, 0x000000000000000fu, 0x000000000000002du, 0x000000000000013bu,
    0x000000000000013bu, 0x0000000000000b13u, 0x000000000000375fu, 0x0000000000026115u,
    0x000000000007233fu, 0x00000000005cca33u, 0x0000000002898765u, 0x00000000260eeeebu,
    0x00000000260eeeebu, 0x0000000286fddd9bu, 0x00000016beecca73u, 0x000001b02b930689u,
    0x00000870d9df20adu, 0x0000b141df4dae31u, 0x00079dd498567c1bu, 0x00af2e19afc5266du,
    0x020d8a4d0f4f7347u, 0x335281867ec241efu, 0x9b3093d46fdd5923u, 0x5e1f9767cc5866b1u,
    0x92dd23d6966aced7u, 0xa30d0f4f0a196e5bu, 0x8dc3e5a1977d7755u, 0x2ab8ce915831734bu,
    0x2ab8ce915831734bu, 0x81d2a0bc5e5fdcabu, 0x9efcac82445da75bu, 0xbc8b95cf58cde171u,
    0xa0e8444a1f3cecf9u, 0x4191deb683ce3ffdu, 0xddd3878bc84ebfc7u, 0xcb39a64b83ff3751u,
    0xf8203f7993fc1495u, 0xbd2a2a78b35f4bddu, 0x84757be6b6d13921u, 0x3fbbcfc0b524988bu,
    0xbd11ed47c8928df9u, 0x3c26b59e41c2f4c5u, 0x677a5137e883fdb3u, 0xff74e943b03b93ddu,
    0xfe5ebbcb10b2bb97u, 0xb021f1de3235e7e7u, 0x33509eb2e743a58fu, 0x390f9da41279fb7du,
    0xe5cb0154f031c559u, 0x93074695ba4ddb6du, 0x81c471caa636247fu, 0xe1347289b5a1d749u,
    0x286f21c3f76ce2ffu, 0x00be84a2173e8ac7u, 0x1595065ca215b88bu, 0xf95877595b018809u,
    0x9c2efe3c5516f887u, 0x373294604679382bu, 0xaf1ff7a888adcd35u, 0x18ddf279a2c5800bu,
    0x18ddf279a2c5800bu, 0x505a90e2542582cbu, 0x5bacad2cd8d5dc2bu, 0xfe3152bcbff89f41u,
    0xe1467e88bf829351u, 0xb8001adb9e31b4d5u, 0x2803ac06a0cbb91fu, 0x1904b5d698805799u,
    0xe12a648b5c831461u, 0x3516abbd6160cfa9u, 0xac46d25f12fe036du, 0x78bfa1da906b00efu,
    0xf6390338b7f111bdu, 0x0f25f80f538255d9u, 0x4ec8ca55b8db140fu, 0x4ff670740b9b30a1u,
    0x8fd032443a07f325u, 0x80dfe7965c83eeb5u, 0xa3dc1714d1213afdu, 0x205b7bbfcdc62007u,
    0xa78126bbe140a093u, 0x9de1dc61ca7550cfu, 0x84f0046d01b492c5u, 0x2d91810b945de0f3u,
    0xf5408b7f6008aa71u, 0x43707f4863034149u, 0xdac65fb9679279d5u, 0xc48406e7d1114eb7u,
    0xa7dc9ed3c88e1271u, 0xfb25b2efdb9cb30du, 0x1bebda0951c4df63u, 0x5c85e975580ee5bdu,
    0x1591bc60082cb137u, 0x2c38606318ef25d7u, 0x76ca72f7c5c63e27u, 0xf04a75d17baa0915u,
    0x77458175139ae30du, 0x0e6c1330bc1b9421u, 0xdf87d2b5797e8293u, 0xefa5c703e1e68925u,
    0x2b6b1b3278b4f6e1u, 0xceee27b382394249u, 0xd74e3829f5dab91du, 0xfdb17989c26b5f1fu,
    0xc1b7d18781530845u, 0x7b4436b2105a8561u, 0x7ba7c0418372a7d7u, 0x9dbc5c67feb6c639u,
    0x502686d7f6ff6b8fu, 0x6101855406be7a1fu, 0x9956afb5806930e7u, 0xe1f0ee88af40f7c5u,
    0x984b057bda5c1151u, 0x9a49819acc13ea05u, 0x8ef0dead0896ef27u, 0x71f7826efe292b21u,
    0xad80a480e46986efu, 0x01cdc0ebf5e0c6f7u, 0x6e06f839968f68dbu, 0xdd5943ab56e76139u,
    0xcdcf31bf8604c5e7u, 0x7e2b4a847054a1cbu, 0x0ca75697a4d3d0f5u, 0x4703f53ac514a98bu,
};

/* inverses of reduced_factorial_odd_part values modulo 2**64.

Python code to generate the values:

    import math.integer

    for n in range(128):
        fac = math.integer.factorial(n)
        fac_odd_part = fac // (fac & -fac)
        inverted_fac_odd_part = pow(fac_odd_part, -1, 2**64)
        print(f"{inverted_fac_odd_part:#018x}u")
*/
static const uint64_t inverted_factorial_odd_part[] = {
    0x0000000000000001u, 0x0000000000000001u, 0x0000000000000001u, 0xaaaaaaaaaaaaaaabu,
    0xaaaaaaaaaaaaaaabu, 0xeeeeeeeeeeeeeeefu, 0x4fa4fa4fa4fa4fa5u, 0x2ff2ff2ff2ff2ff3u,
    0x2ff2ff2ff2ff2ff3u, 0x938cc70553e3771bu, 0xb71c27cddd93e49fu, 0xb38e3229fcdee63du,
    0xe684bb63544a4cbfu, 0xc2f684917ca340fbu, 0xf747c9cba417526du, 0xbb26eb51d7bd49c3u,
    0xbb26eb51d7bd49c3u, 0xb0a7efb985294093u, 0xbe4b8c69f259eabbu, 0x6854d17ed6dc4fb9u,
    0xe1aa904c915f4325u, 0x3b8206df131cead1u, 0x79c6009fea76fe13u, 0xd8c5d381633cd365u,
    0x4841f12b21144677u, 0x4a91ff68200b0d0fu, 0x8f9513a58c4f9e8bu, 0x2b3e690621a42251u,
    0x4f520f00e03c04e7u, 0x2edf84ee600211d3u, 0xadcaa2764aaacdfdu, 0x161f4f9033f4fe63u,
    0x161f4f9033f4fe63u, 0xbada2932ea4d3e03u, 0xcec189f3efaa30d3u, 0xf7475bb68330bf91u,
    0x37eb7bf7d5b01549u, 0x46b35660a4e91555u, 0xa567c12d81f151f7u, 0x4c724007bb2071b1u,
    0x0f4a0cce58a016bdu, 0xfa21068e66106475u, 0x244ab72b5a318ae1u, 0x366ce67e080d0f23u,
    0xd666fdae5dd2a449u, 0xd740ddd0acc06a0du, 0xb050bbbb28e6f97bu, 0x70b003fe890a5c75u,
    0xd03aabff83037427u, 0x13ec4ca72c783bd7u, 0x90282c06afdbd96fu, 0x4414ddb9db4a95d5u,
    0xa2c68735ae6832e9u, 0xbf72d71455676665u, 0xa8469fab6b759b7fu, 0xc1e55b56e606caf9u,
    0x40455630fc4a1cffu, 0x0120a7b0046d16f7u, 0xa7c3553b08faef23u, 0x9f0bfd1b08d48639u,
    0xa433ffce9a304d37u, 0xa22ad1d53915c683u, 0xcb6cbc723ba5dd1du, 0x547fb1b8ab9d0ba3u,
    0x547fb1b8ab9d0ba3u, 0x8f15a826498852e3u, 0x32e1a03f38880283u, 0x3de4cce63283f0c1u,
    0x5dfe6667e4da95b1u, 0xfda6eeeef479e47du, 0xf14de991cc7882dfu, 0xe68db79247630ca9u,
    0xa7d6db8207ee8fa1u, 0x255e1f0fcf034499u, 0xc9a8990e43dd7e65u, 0x3279b6f289702e0fu,
    0xe7b5905d9b71b195u, 0x03025ba41ff0da69u, 0xb7df3d6d3be55aefu, 0xf89b212ebff2b361u,
    0xfe856d095996f0adu, 0xd6e533e9fdf20f9du, 0xf8c0e84a63da3255u, 0xa677876cd91b4db7u,
    0x07ed4f97780d7d9bu, 0x90a8705f258db62fu, 0xa41bbb2be31b1c0du, 0x6ec28690b038383bu,
    0xdb860c3bb2edd691u, 0x0838286838a980f9u, 0x558417a74b36f77du, 0x71779afc3646ef07u,
    0x743cda377ccb6e91u, 0x7fdf9f3fe89153c5u, 0xdc97d25df49b9a4bu, 0x76321a778eb37d95u,
    0x7cbb5e27da3bd487u, 0x9cff4ade1a009de7u, 0x70eb166d05c15197u, 0xdcf0460b71d5fe3du,
    0x5ac1ee5260b6a3c5u, 0xc922dedfdd78efe1u, 0xe5d381dc3b8eeb9bu, 0xd57e5347bafc6aadu,
    0x86939040983acd21u, 0x395b9d69740a4ff9u, 0x1467299c8e43d135u, 0x5fe440fcad975cdfu,
    0xcaa9a39794a6ca8du, 0xf61dbd640868dea1u, 0xac09d98d74843be7u, 0x2b103b9e1a6b4809u,
    0x2ab92d16960f536fu, 0x6653323d5e3681dfu, 0xefd48c1c0624e2d7u, 0xa496fefe04816f0du,
    0x1754a7b07bbdd7b1u, 0x23353c829a3852cdu, 0xbf831261abd59097u, 0x57a8e656df0618e1u,
    0x16e9206c3100680fu, 0xadad4c6ee921dac7u, 0x635f2b3860265353u, 0xdd6d0059f44b3d09u,
    0xac4dd6b894447dd7u, 0x42ea183eeaa87be3u, 0x15612d1550ee5b5du, 0x226fa19d656cb623u,
};

/* exponent of the largest power of 2 dividing factorial(n), for n in range(68)

Python code to generate the values:

import math.integer

for n in range(128):
    fac = math.integer.factorial(n)
    fac_trailing_zeros = (fac & -fac).bit_length() - 1
    print(fac_trailing_zeros)
*/

static const uint8_t factorial_trailing_zeros[] = {
     0,  0,  1,  1,  3,  3,  4,  4,  7,  7,  8,  8, 10, 10, 11, 11,  //  0-15
    15, 15, 16, 16, 18, 18, 19, 19, 22, 22, 23, 23, 25, 25, 26, 26,  // 16-31
    31, 31, 32, 32, 34, 34, 35, 35, 38, 38, 39, 39, 41, 41, 42, 42,  // 32-47
    46, 46, 47, 47, 49, 49, 50, 50, 53, 53, 54, 54, 56, 56, 57, 57,  // 48-63
    63, 63, 64, 64, 66, 66, 67, 67, 70, 70, 71, 71, 73, 73, 74, 74,  // 64-79
    78, 78, 79, 79, 81, 81, 82, 82, 85, 85, 86, 86, 88, 88, 89, 89,  // 80-95
    94, 94, 95, 95, 97, 97, 98, 98, 101, 101, 102, 102, 104, 104, 105, 105,  // 96-111
    109, 109, 110, 110, 112, 112, 113, 113, 116, 116, 117, 117, 119, 119, 120, 120,  // 112-127
};

/* Number of permutations and combinations.
 * P(n, k) = n! / (n-k)!
 * C(n, k) = P(n, k) / k!
 */

/* Calculate C(n, k) for n in the 63-bit range. */
static PyObject *
perm_comb_small(unsigned long long n, unsigned long long k, int iscomb)
{
    assert(k != 0);

    /* For small enough n and k the result fits in the 64-bit range and can
     * be calculated without allocating intermediate PyLong objects. */
    if (iscomb) {
        /* Maps k to the maximal n so that 2*k-1 <= n <= 127 and C(n, k)
         * fits into a uint64_t.  Exclude k = 1, because the second fast
         * path is faster for this case.*/
        static const unsigned char fast_comb_limits1[] = {
            0, 0, 127, 127, 127, 127, 127, 127,  // 0-7
            127, 127, 127, 127, 127, 127, 127, 127,  // 8-15
            116, 105, 97, 91, 86, 82, 78, 76,  // 16-23
            74, 72, 71, 70, 69, 68, 68, 67,  // 24-31
            67, 67, 67,  // 32-34
        };
        if (k < Py_ARRAY_LENGTH(fast_comb_limits1) && n <= fast_comb_limits1[k]) {
            /*
                comb(n, k) fits into a uint64_t. We compute it as

                    comb_odd_part << shift

                where 2**shift is the largest power of two dividing comb(n, k)
                and comb_odd_part is comb(n, k) >> shift. comb_odd_part can be
                calculated efficiently via arithmetic modulo 2**64, using three
                lookups and two uint64_t multiplications.
            */
            uint64_t comb_odd_part = reduced_factorial_odd_part[n]
                                   * inverted_factorial_odd_part[k]
                                   * inverted_factorial_odd_part[n - k];
            int shift = factorial_trailing_zeros[n]
                      - factorial_trailing_zeros[k]
                      - factorial_trailing_zeros[n - k];
            return PyLong_FromUnsignedLongLong(comb_odd_part << shift);
        }

        /* Maps k to the maximal n so that 2*k-1 <= n <= 127 and C(n, k)*k
         * fits into a long long (which is at least 64 bit).  Only contains
         * items larger than in fast_comb_limits1. */
        static const unsigned long long fast_comb_limits2[] = {
            0, ULLONG_MAX, 4294967296ULL, 3329022, 102570, 13467, 3612, 1449,  // 0-7
            746, 453, 308, 227, 178, 147,  // 8-13
        };
        if (k < Py_ARRAY_LENGTH(fast_comb_limits2) && n <= fast_comb_limits2[k]) {
            /* C(n, k) = C(n, k-1) * (n-k+1) / k */
            unsigned long long result = n;
            for (unsigned long long i = 1; i < k;) {
                result *= --n;
                result /= ++i;
            }
            return PyLong_FromUnsignedLongLong(result);
        }
    }
    else {
        /* Maps k to the maximal n so that k <= n and P(n, k)
         * fits into a long long (which is at least 64 bit). */
        static const unsigned long long fast_perm_limits[] = {
            0, ULLONG_MAX, 4294967296ULL, 2642246, 65537, 7133, 1627, 568,  // 0-7
            259, 142, 88, 61, 45, 36, 30, 26,  // 8-15
            24, 22, 21, 20, 20,  // 16-20
        };
        if (k < Py_ARRAY_LENGTH(fast_perm_limits) && n <= fast_perm_limits[k]) {
            if (n <= 127) {
                /* P(n, k) fits into a uint64_t. */
                uint64_t perm_odd_part = reduced_factorial_odd_part[n]
                                       * inverted_factorial_odd_part[n - k];
                int shift = factorial_trailing_zeros[n]
                          - factorial_trailing_zeros[n - k];
                return PyLong_FromUnsignedLongLong(perm_odd_part << shift);
            }

            /* P(n, k) = P(n, k-1) * (n-k+1) */
            unsigned long long result = n;
            for (unsigned long long i = 1; i < k;) {
                result *= --n;
                ++i;
            }
            return PyLong_FromUnsignedLongLong(result);
        }
    }

    /* For larger n use recursive formulas:
     *
     *   P(n, k) = P(n, j) * P(n-j, k-j)
     *   C(n, k) = C(n, j) * C(n-j, k-j) // C(k, j)
     */
    unsigned long long j = k / 2;
    PyObject *a, *b;
    a = perm_comb_small(n, j, iscomb);
    if (a == NULL) {
        return NULL;
    }
    b = perm_comb_small(n - j, k - j, iscomb);
    if (b == NULL) {
        goto error;
    }
    Py_SETREF(a, PyNumber_Multiply(a, b));
    Py_DECREF(b);
    if (iscomb && a != NULL) {
        b = perm_comb_small(k, j, 1);
        if (b == NULL) {
            goto error;
        }
        Py_SETREF(a, PyNumber_FloorDivide(a, b));
        Py_DECREF(b);
    }
    return a;

error:
    Py_DECREF(a);
    return NULL;
}

/* Calculate P(n, k) or C(n, k) using recursive formulas.
 * It is more efficient than sequential multiplication thanks to
 * Karatsuba multiplication.
 */
static PyObject *
perm_comb(PyObject *n, unsigned long long k, int iscomb)
{
    if (k == 0) {
        return PyLong_FromLong(1);
    }
    if (k == 1) {
        return Py_NewRef(n);
    }

    /* P(n, k) = P(n, j) * P(n-j, k-j) */
    /* C(n, k) = C(n, j) * C(n-j, k-j) // C(k, j) */
    unsigned long long j = k / 2;
    PyObject *a, *b;
    a = perm_comb(n, j, iscomb);
    if (a == NULL) {
        return NULL;
    }
    PyObject *t = PyLong_FromUnsignedLongLong(j);
    if (t == NULL) {
        goto error;
    }
    n = PyNumber_Subtract(n, t);
    Py_DECREF(t);
    if (n == NULL) {
        goto error;
    }
    b = perm_comb(n, k - j, iscomb);
    Py_DECREF(n);
    if (b == NULL) {
        goto error;
    }
    Py_SETREF(a, PyNumber_Multiply(a, b));
    Py_DECREF(b);
    if (iscomb && a != NULL) {
        b = perm_comb_small(k, j, 1);
        if (b == NULL) {
            goto error;
        }
        Py_SETREF(a, PyNumber_FloorDivide(a, b));
        Py_DECREF(b);
    }
    return a;

error:
    Py_DECREF(a);
    return NULL;
}

/*[clinic input]
@permit_long_summary
math.integer.perm

    n: object
    k: object = None
    /

Number of ways to choose k items from n items without repetition and with order.

Evaluates to n! / (n - k)! when k <= n and evaluates
to zero when k > n.

If k is not specified or is None, then k defaults to n
and the function returns n!.

Raises ValueError if either of the arguments are negative.
[clinic start generated code]*/

static PyObject *
math_integer_perm_impl(PyObject *module, PyObject *n, PyObject *k)
/*[clinic end generated code: output=9f9b96cd73a94de4 input=fd627e5a09dd5116]*/
{
    PyObject *result = NULL;
    int overflow, cmp;
    long long ki, ni;

    if (k == Py_None) {
        return math_integer_factorial(module, n);
    }
    n = PyNumber_Index(n);
    if (n == NULL) {
        return NULL;
    }
    k = PyNumber_Index(k);
    if (k == NULL) {
        Py_DECREF(n);
        return NULL;
    }
    assert(PyLong_CheckExact(n) && PyLong_CheckExact(k));

    if (_PyLong_IsNegative((PyLongObject *)n)) {
        PyErr_SetString(PyExc_ValueError,
                        "n must be a non-negative integer");
        goto error;
    }
    if (_PyLong_IsNegative((PyLongObject *)k)) {
        PyErr_SetString(PyExc_ValueError,
                        "k must be a non-negative integer");
        goto error;
    }

    cmp = PyObject_RichCompareBool(n, k, Py_LT);
    if (cmp != 0) {
        if (cmp > 0) {
            result = PyLong_FromLong(0);
            goto done;
        }
        goto error;
    }

    ki = PyLong_AsLongLongAndOverflow(k, &overflow);
    assert(overflow >= 0 && !PyErr_Occurred());
    if (overflow > 0) {
        PyErr_Format(PyExc_OverflowError,
                     "k must not exceed %lld",
                     LLONG_MAX);
        goto error;
    }
    assert(ki >= 0);

    ni = PyLong_AsLongLongAndOverflow(n, &overflow);
    assert(overflow >= 0 && !PyErr_Occurred());
    if (!overflow && ki > 1) {
        assert(ni >= 0);
        result = perm_comb_small((unsigned long long)ni,
                                 (unsigned long long)ki, 0);
    }
    else {
        result = perm_comb(n, (unsigned long long)ki, 0);
    }

done:
    Py_DECREF(n);
    Py_DECREF(k);
    return result;

error:
    Py_DECREF(n);
    Py_DECREF(k);
    return NULL;
}

/*[clinic input]
@permit_long_summary
math.integer.comb

    n: object
    k: object
    /

Number of ways to choose k items from n items without repetition and without order.

Evaluates to n! / (k! * (n - k)!) when k <= n and evaluates
to zero when k > n.

Also called the binomial coefficient because it is equivalent
to the coefficient of k-th term in polynomial expansion of the
expression (1 + x)**n.

Raises ValueError if either of the arguments are negative.
[clinic start generated code]*/

static PyObject *
math_integer_comb_impl(PyObject *module, PyObject *n, PyObject *k)
/*[clinic end generated code: output=c2c9cdfe0d5dd43f input=8cc12726b682c4a5]*/
{
    PyObject *result = NULL, *temp;
    int overflow, cmp;
    long long ki, ni;

    n = PyNumber_Index(n);
    if (n == NULL) {
        return NULL;
    }
    k = PyNumber_Index(k);
    if (k == NULL) {
        Py_DECREF(n);
        return NULL;
    }
    assert(PyLong_CheckExact(n) && PyLong_CheckExact(k));

    if (_PyLong_IsNegative((PyLongObject *)n)) {
        PyErr_SetString(PyExc_ValueError,
                        "n must be a non-negative integer");
        goto error;
    }
    if (_PyLong_IsNegative((PyLongObject *)k)) {
        PyErr_SetString(PyExc_ValueError,
                        "k must be a non-negative integer");
        goto error;
    }

    ni = PyLong_AsLongLongAndOverflow(n, &overflow);
    assert(overflow >= 0 && !PyErr_Occurred());
    if (!overflow) {
        assert(ni >= 0);
        ki = PyLong_AsLongLongAndOverflow(k, &overflow);
        assert(overflow >= 0 && !PyErr_Occurred());
        if (overflow || ki > ni) {
            result = PyLong_FromLong(0);
            goto done;
        }
        assert(ki >= 0);

        ki = Py_MIN(ki, ni - ki);
        if (ki > 1) {
            result = perm_comb_small((unsigned long long)ni,
                                     (unsigned long long)ki, 1);
            goto done;
        }
        /* For k == 1 just return the original n in perm_comb(). */
    }
    else {
        /* k = min(k, n - k) */
        temp = PyNumber_Subtract(n, k);
        if (temp == NULL) {
            goto error;
        }
        assert(PyLong_Check(temp));
        if (_PyLong_IsNegative((PyLongObject *)temp)) {
            Py_DECREF(temp);
            result = PyLong_FromLong(0);
            goto done;
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

        ki = PyLong_AsLongLongAndOverflow(k, &overflow);
        assert(overflow >= 0 && !PyErr_Occurred());
        if (overflow) {
            PyErr_Format(PyExc_OverflowError,
                         "min(n - k, k) must not exceed %lld",
                         LLONG_MAX);
            goto error;
        }
        assert(ki >= 0);
    }

    result = perm_comb(n, (unsigned long long)ki, 1);

done:
    Py_DECREF(n);
    Py_DECREF(k);
    return result;

error:
    Py_DECREF(n);
    Py_DECREF(k);
    return NULL;
}


static PyMethodDef math_integer_methods[] = {
    MATH_INTEGER_COMB_METHODDEF
    MATH_INTEGER_FACTORIAL_METHODDEF
    MATH_INTEGER_GCD_METHODDEF
    MATH_INTEGER_ISQRT_METHODDEF
    MATH_INTEGER_LCM_METHODDEF
    MATH_INTEGER_PERM_METHODDEF
    {NULL,              NULL}           /* sentinel */
};

static int
math_integer_exec(PyObject *module)
{
    /* Fix the __name__ attribute of the module and the __module__ attribute
     * of its functions.
     */
    PyObject *name = PyUnicode_FromString("math.integer");
    if (name == NULL) {
        return -1;
    }
    if (PyObject_SetAttrString(module, "__name__", name) < 0) {
        Py_DECREF(name);
        return -1;
    }
    for (const PyMethodDef *m = math_integer_methods; m->ml_name; m++) {
        PyObject *obj = PyObject_GetAttrString(module, m->ml_name);
        if (obj == NULL) {
            Py_DECREF(name);
            return -1;
        }
        if (PyObject_SetAttrString(obj, "__module__", name) < 0) {
            Py_DECREF(name);
            Py_DECREF(obj);
            return -1;
        }
        Py_DECREF(obj);
    }
    Py_DECREF(name);
    return 0;
}

static PyModuleDef_Slot math_integer_slots[] = {
    {Py_mod_exec, math_integer_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

PyDoc_STRVAR(module_doc,
"This module provides access to integer related mathematical functions.");

static struct PyModuleDef math_integer_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "math.integer",
    .m_doc = module_doc,
    .m_size = 0,
    .m_methods = math_integer_methods,
    .m_slots = math_integer_slots,
};

PyMODINIT_FUNC
PyInit__math_integer(void)
{
    return PyModuleDef_Init(&math_integer_module);
}
