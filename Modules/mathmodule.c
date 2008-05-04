/* Math module -- standard C math library functions, pi and e */

/* Here are some comments from Tim Peters, extracted from the
   discussion attached to http://bugs.python.org/issue1640.  They
   describe the general aims of the math module with respect to
   special values, IEEE-754 floating-point exceptions, and Python
   exceptions.

These are the "spirit of 754" rules:

1. If the mathematical result is a real number, but of magnitude too
large to approximate by a machine float, overflow is signaled and the
result is an infinity (with the appropriate sign).

2. If the mathematical result is a real number, but of magnitude too
small to approximate by a machine float, underflow is signaled and the
result is a zero (with the appropriate sign).

3. At a singularity (a value x such that the limit of f(y) as y
approaches x exists and is an infinity), "divide by zero" is signaled
and the result is an infinity (with the appropriate sign).  This is
complicated a little by that the left-side and right-side limits may
not be the same; e.g., 1/x approaches +inf or -inf as x approaches 0
from the positive or negative directions.  In that specific case, the
sign of the zero determines the result of 1/0.

4. At a point where a function has no defined result in the extended
reals (i.e., the reals plus an infinity or two), invalid operation is
signaled and a NaN is returned.

And these are what Python has historically /tried/ to do (but not
always successfully, as platform libm behavior varies a lot):

For #1, raise OverflowError.

For #2, return a zero (with the appropriate sign if that happens by
accident ;-)).

For #3 and #4, raise ValueError.  It may have made sense to raise
Python's ZeroDivisionError in #3, but historically that's only been
raised for division by zero and mod by zero.

*/

/*
   In general, on an IEEE-754 platform the aim is to follow the C99
   standard, including Annex 'F', whenever possible.  Where the
   standard recommends raising the 'divide-by-zero' or 'invalid'
   floating-point exceptions, Python should raise a ValueError.  Where
   the standard recommends raising 'overflow', Python should raise an
   OverflowError.  In all other circumstances a value should be
   returned.
 */

#include "Python.h"
#include "longintrepr.h" /* just for SHIFT */

#ifdef _OSF_SOURCE
/* OSF1 5.1 doesn't make this available with XOPEN_SOURCE_EXTENDED defined */
extern double copysign(double, double);
#endif

/* Call is_error when errno != 0, and where x is the result libm
 * returned.  is_error will usually set up an exception and return
 * true (1), but may return false (0) without setting up an exception.
 */
static int
is_error(double x)
{
	int result = 1;	/* presumption of guilt */
	assert(errno);	/* non-zero errno is a precondition for calling */
	if (errno == EDOM)
		PyErr_SetString(PyExc_ValueError, "math domain error");

	else if (errno == ERANGE) {
		/* ANSI C generally requires libm functions to set ERANGE
		 * on overflow, but also generally *allows* them to set
		 * ERANGE on underflow too.  There's no consistency about
		 * the latter across platforms.
		 * Alas, C99 never requires that errno be set.
		 * Here we suppress the underflow errors (libm functions
		 * should return a zero on underflow, and +- HUGE_VAL on
		 * overflow, so testing the result for zero suffices to
		 * distinguish the cases).
		 */
		if (x)
			PyErr_SetString(PyExc_OverflowError,
					"math range error");
		else
			result = 0;
	}
	else
                /* Unexpected math error */
		PyErr_SetFromErrno(PyExc_ValueError);
	return result;
}

/*
   wrapper for atan2 that deals directly with special cases before
   delegating to the platform libm for the remaining cases.  This
   is necessary to get consistent behaviour across platforms.
   Windows, FreeBSD and alpha Tru64 are amongst platforms that don't
   always follow C99.
*/

static double
m_atan2(double y, double x)
{
	if (Py_IS_NAN(x) || Py_IS_NAN(y))
		return Py_NAN;
	if (Py_IS_INFINITY(y)) {
		if (Py_IS_INFINITY(x)) {
			if (copysign(1., x) == 1.)
				/* atan2(+-inf, +inf) == +-pi/4 */
				return copysign(0.25*Py_MATH_PI, y);
			else
				/* atan2(+-inf, -inf) == +-pi*3/4 */
				return copysign(0.75*Py_MATH_PI, y);
		}
		/* atan2(+-inf, x) == +-pi/2 for finite x */
		return copysign(0.5*Py_MATH_PI, y);
	}
	if (Py_IS_INFINITY(x) || y == 0.) {
		if (copysign(1., x) == 1.)
			/* atan2(+-y, +inf) = atan2(+-0, +x) = +-0. */
			return copysign(0., y);
		else
			/* atan2(+-y, -inf) = atan2(+-0., -x) = +-pi. */
			return copysign(Py_MATH_PI, y);
	}
	return atan2(y, x);
}

/*
   math_1 is used to wrap a libm function f that takes a double
   arguments and returns a double.

   The error reporting follows these rules, which are designed to do
   the right thing on C89/C99 platforms and IEEE 754/non IEEE 754
   platforms.

   - a NaN result from non-NaN inputs causes ValueError to be raised
   - an infinite result from finite inputs causes OverflowError to be
     raised if can_overflow is 1, or raises ValueError if can_overflow
     is 0.
   - if the result is finite and errno == EDOM then ValueError is
     raised
   - if the result is finite and nonzero and errno == ERANGE then
     OverflowError is raised

   The last rule is used to catch overflow on platforms which follow
   C89 but for which HUGE_VAL is not an infinity.

   For the majority of one-argument functions these rules are enough
   to ensure that Python's functions behave as specified in 'Annex F'
   of the C99 standard, with the 'invalid' and 'divide-by-zero'
   floating-point exceptions mapping to Python's ValueError and the
   'overflow' floating-point exception mapping to OverflowError.
   math_1 only works for functions that don't have singularities *and*
   the possibility of overflow; fortunately, that covers everything we
   care about right now.
*/

static PyObject *
math_1_to_whatever(PyObject *arg, double (*func) (double),
                   PyObject *(*from_double_func) (double),
                   int can_overflow)
{
	double x, r;
	char err_message[150];
	x = PyFloat_AsDouble(arg);
	if (x == -1.0 && PyErr_Occurred())
		return NULL;
	errno = 0;
	PyFPE_START_PROTECT("in math_1", return 0);
	r = (*func)(x);
	PyFPE_END_PROTECT(r);
	if (Py_IS_NAN(r) && !Py_IS_NAN(x)) {
		PyErr_SetString(PyExc_ValueError,
				"math domain error (invalid argument)");
		return NULL;
	}
	if (Py_IS_INFINITY(r) && Py_IS_FINITE(x)) {
			if (can_overflow)
				PyErr_SetString(PyExc_OverflowError,
					"math range error (overflow)");
			else {
				/* temporary code to include the inputs
				   and outputs to func in the error
				   message */
				sprintf(err_message,
					"math domain error (singularity) "
					"%.17g -> %.17g",
					x, r);
				PyErr_SetString(PyExc_ValueError, err_message);
			}
			return NULL;
	}
	if (Py_IS_FINITE(r) && errno && is_error(r))
		/* this branch unnecessary on most platforms */
		return NULL;

	return (*from_double_func)(r);
}

/*
   math_2 is used to wrap a libm function f that takes two double
   arguments and returns a double.

   The error reporting follows these rules, which are designed to do
   the right thing on C89/C99 platforms and IEEE 754/non IEEE 754
   platforms.

   - a NaN result from non-NaN inputs causes ValueError to be raised
   - an infinite result from finite inputs causes OverflowError to be
     raised.
   - if the result is finite and errno == EDOM then ValueError is
     raised
   - if the result is finite and nonzero and errno == ERANGE then
     OverflowError is raised

   The last rule is used to catch overflow on platforms which follow
   C89 but for which HUGE_VAL is not an infinity.

   For most two-argument functions (copysign, fmod, hypot, atan2)
   these rules are enough to ensure that Python's functions behave as
   specified in 'Annex F' of the C99 standard, with the 'invalid' and
   'divide-by-zero' floating-point exceptions mapping to Python's
   ValueError and the 'overflow' floating-point exception mapping to
   OverflowError.
*/

static PyObject *
math_1(PyObject *arg, double (*func) (double), int can_overflow)
{
	return math_1_to_whatever(arg, func, PyFloat_FromDouble, can_overflow);
}

static PyObject *
math_1_to_int(PyObject *arg, double (*func) (double), int can_overflow)
{
	return math_1_to_whatever(arg, func, PyLong_FromDouble, can_overflow);
}

static PyObject *
math_2(PyObject *args, double (*func) (double, double), char *funcname)
{
	PyObject *ox, *oy;
	double x, y, r;
	if (! PyArg_UnpackTuple(args, funcname, 2, 2, &ox, &oy))
		return NULL;
	x = PyFloat_AsDouble(ox);
	y = PyFloat_AsDouble(oy);
	if ((x == -1.0 || y == -1.0) && PyErr_Occurred())
		return NULL;
	errno = 0;
	PyFPE_START_PROTECT("in math_2", return 0);
	r = (*func)(x, y);
	PyFPE_END_PROTECT(r);
	if (Py_IS_NAN(r)) {
		if (!Py_IS_NAN(x) && !Py_IS_NAN(y))
			errno = EDOM;
		else
			errno = 0;
	}
	else if (Py_IS_INFINITY(r)) {
		if (Py_IS_FINITE(x) && Py_IS_FINITE(y))
			errno = ERANGE;
		else
			errno = 0;
	}
	if (errno && is_error(r))
		return NULL;
	else
		return PyFloat_FromDouble(r);
}

#define FUNC1(funcname, func, can_overflow, docstring)			\
	static PyObject * math_##funcname(PyObject *self, PyObject *args) { \
		return math_1(args, func, can_overflow);		    \
	}\
        PyDoc_STRVAR(math_##funcname##_doc, docstring);

#define FUNC2(funcname, func, docstring) \
	static PyObject * math_##funcname(PyObject *self, PyObject *args) { \
		return math_2(args, func, #funcname); \
	}\
        PyDoc_STRVAR(math_##funcname##_doc, docstring);

FUNC1(acos, acos, 0,
      "acos(x)\n\nReturn the arc cosine (measured in radians) of x.")
FUNC1(acosh, acosh, 0,
      "acosh(x)\n\nReturn the hyperbolic arc cosine (measured in radians) of x.")
FUNC1(asin, asin, 0,
      "asin(x)\n\nReturn the arc sine (measured in radians) of x.")
FUNC1(asinh, asinh, 0,
      "asinh(x)\n\nReturn the hyperbolic arc sine (measured in radians) of x.")
FUNC1(atan, atan, 0,
      "atan(x)\n\nReturn the arc tangent (measured in radians) of x.")
FUNC2(atan2, m_atan2,
      "atan2(y, x)\n\nReturn the arc tangent (measured in radians) of y/x.\n"
      "Unlike atan(y/x), the signs of both x and y are considered.")
FUNC1(atanh, atanh, 0,
      "atanh(x)\n\nReturn the hyperbolic arc tangent (measured in radians) of x.")

static PyObject * math_ceil(PyObject *self, PyObject *number) {
	static PyObject *ceil_str = NULL;
	PyObject *method;

	if (ceil_str == NULL) {
		ceil_str = PyUnicode_InternFromString("__ceil__");
		if (ceil_str == NULL)
			return NULL;
	}

	method = _PyType_Lookup(Py_TYPE(number), ceil_str);
	if (method == NULL)
		return math_1_to_int(number, ceil, 0);
	else
		return PyObject_CallFunction(method, "O", number);
}

PyDoc_STRVAR(math_ceil_doc,
	     "ceil(x)\n\nReturn the ceiling of x as an int.\n"
	     "This is the smallest integral value >= x.");

FUNC2(copysign, copysign,
      "copysign(x,y)\n\nReturn x with the sign of y.")
FUNC1(cos, cos, 0,
      "cos(x)\n\nReturn the cosine of x (measured in radians).")
FUNC1(cosh, cosh, 1,
      "cosh(x)\n\nReturn the hyperbolic cosine of x.")
FUNC1(exp, exp, 1,
      "exp(x)\n\nReturn e raised to the power of x.")
FUNC1(fabs, fabs, 0,
      "fabs(x)\n\nReturn the absolute value of the float x.")

static PyObject * math_floor(PyObject *self, PyObject *number) {
	static PyObject *floor_str = NULL;
	PyObject *method;

	if (floor_str == NULL) {
		floor_str = PyUnicode_InternFromString("__floor__");
		if (floor_str == NULL)
			return NULL;
	}

	method = _PyType_Lookup(Py_TYPE(number), floor_str);
	if (method == NULL)
        	return math_1_to_int(number, floor, 0);
	else
		return PyObject_CallFunction(method, "O", number);
}

PyDoc_STRVAR(math_floor_doc,
	     "floor(x)\n\nReturn the floor of x as an int.\n"
	     "This is the largest integral value <= x.");

FUNC1(log1p, log1p, 1,
      "log1p(x)\n\nReturn the natural logarithm of 1+x (base e).\n\
      The result is computed in a way which is accurate for x near zero.")
FUNC1(sin, sin, 0,
      "sin(x)\n\nReturn the sine of x (measured in radians).")
FUNC1(sinh, sinh, 1,
      "sinh(x)\n\nReturn the hyperbolic sine of x.")
FUNC1(sqrt, sqrt, 0,
      "sqrt(x)\n\nReturn the square root of x.")
FUNC1(tan, tan, 0,
      "tan(x)\n\nReturn the tangent of x (measured in radians).")
FUNC1(tanh, tanh, 0,
      "tanh(x)\n\nReturn the hyperbolic tangent of x.")

static PyObject *
math_trunc(PyObject *self, PyObject *number)
{
	static PyObject *trunc_str = NULL;
	PyObject *trunc;

	if (Py_TYPE(number)->tp_dict == NULL) {
		if (PyType_Ready(Py_TYPE(number)) < 0)
			return NULL;
	}

	if (trunc_str == NULL) {
		trunc_str = PyUnicode_InternFromString("__trunc__");
		if (trunc_str == NULL)
			return NULL;
	}

	trunc = _PyType_Lookup(Py_TYPE(number), trunc_str);
	if (trunc == NULL) {
		PyErr_Format(PyExc_TypeError,
			     "type %.100s doesn't define __trunc__ method",
			     Py_TYPE(number)->tp_name);
		return NULL;
	}
	return PyObject_CallFunctionObjArgs(trunc, number, NULL);
}

PyDoc_STRVAR(math_trunc_doc,
"trunc(x:Real) -> Integral\n"
"\n"
"Truncates x to the nearest Integral toward 0. Uses the __trunc__ magic method.");

static PyObject *
math_frexp(PyObject *self, PyObject *arg)
{
	int i;
	double x = PyFloat_AsDouble(arg);
	if (x == -1.0 && PyErr_Occurred())
		return NULL;
	/* deal with special cases directly, to sidestep platform
	   differences */
	if (Py_IS_NAN(x) || Py_IS_INFINITY(x) || !x) {
		i = 0;
	}
	else {
		PyFPE_START_PROTECT("in math_frexp", return 0);
		x = frexp(x, &i);
		PyFPE_END_PROTECT(x);
	}
	return Py_BuildValue("(di)", x, i);
}

PyDoc_STRVAR(math_frexp_doc,
"frexp(x)\n"
"\n"
"Return the mantissa and exponent of x, as pair (m, e).\n"
"m is a float and e is an int, such that x = m * 2.**e.\n"
"If x is 0, m and e are both 0.  Else 0.5 <= abs(m) < 1.0.");

static PyObject *
math_ldexp(PyObject *self, PyObject *args)
{
	double x, r;
	int exp;
	if (! PyArg_ParseTuple(args, "di:ldexp", &x, &exp))
		return NULL;
	errno = 0;
	PyFPE_START_PROTECT("in math_ldexp", return 0)
	r = ldexp(x, exp);
	PyFPE_END_PROTECT(r)
	if (Py_IS_FINITE(x) && Py_IS_INFINITY(r))
		errno = ERANGE;
	/* Windows MSVC8 sets errno = EDOM on ldexp(NaN, i);
	   we unset it to avoid raising a ValueError here. */
	if (errno == EDOM)
		errno = 0;
	if (errno && is_error(r))
		return NULL;
	else
		return PyFloat_FromDouble(r);
}

PyDoc_STRVAR(math_ldexp_doc,
"ldexp(x, i) -> x * (2**i)");

static PyObject *
math_modf(PyObject *self, PyObject *arg)
{
	double y, x = PyFloat_AsDouble(arg);
	if (x == -1.0 && PyErr_Occurred())
		return NULL;
	/* some platforms don't do the right thing for NaNs and
	   infinities, so we take care of special cases directly. */
	if (!Py_IS_FINITE(x)) {
		if (Py_IS_INFINITY(x))
			return Py_BuildValue("(dd)", copysign(0., x), x);
		else if (Py_IS_NAN(x))
			return Py_BuildValue("(dd)", x, x);
	}          

	errno = 0;
	PyFPE_START_PROTECT("in math_modf", return 0);
	x = modf(x, &y);
	PyFPE_END_PROTECT(x);
	return Py_BuildValue("(dd)", x, y);
}

PyDoc_STRVAR(math_modf_doc,
"modf(x)\n"
"\n"
"Return the fractional and integer parts of x.  Both results carry the sign\n"
"of x.  The integer part is returned as a real.");

/* A decent logarithm is easy to compute even for huge longs, but libm can't
   do that by itself -- loghelper can.  func is log or log10, and name is
   "log" or "log10".  Note that overflow isn't possible:  a long can contain
   no more than INT_MAX * SHIFT bits, so has value certainly less than
   2**(2**64 * 2**16) == 2**2**80, and log2 of that is 2**80, which is
   small enough to fit in an IEEE single.  log and log10 are even smaller.
*/

static PyObject*
loghelper(PyObject* arg, double (*func)(double), char *funcname)
{
	/* If it is long, do it ourselves. */
	if (PyLong_Check(arg)) {
		double x;
		int e;
		x = _PyLong_AsScaledDouble(arg, &e);
		if (x <= 0.0) {
			PyErr_SetString(PyExc_ValueError,
					"math domain error");
			return NULL;
		}
		/* Value is ~= x * 2**(e*PyLong_SHIFT), so the log ~=
		   log(x) + log(2) * e * PyLong_SHIFT.
		   CAUTION:  e*PyLong_SHIFT may overflow using int arithmetic,
		   so force use of double. */
		x = func(x) + (e * (double)PyLong_SHIFT) * func(2.0);
		return PyFloat_FromDouble(x);
	}

	/* Else let libm handle it by itself. */
	return math_1(arg, func, 0);
}

static PyObject *
math_log(PyObject *self, PyObject *args)
{
	PyObject *arg;
	PyObject *base = NULL;
	PyObject *num, *den;
	PyObject *ans;

	if (!PyArg_UnpackTuple(args, "log", 1, 2, &arg, &base))
		return NULL;

	num = loghelper(arg, log, "log");
	if (num == NULL || base == NULL)
		return num;

	den = loghelper(base, log, "log");
	if (den == NULL) {
		Py_DECREF(num);
		return NULL;
	}

	ans = PyNumber_TrueDivide(num, den);
	Py_DECREF(num);
	Py_DECREF(den);
	return ans;
}

PyDoc_STRVAR(math_log_doc,
"log(x[, base]) -> the logarithm of x to the given base.\n\
If the base not specified, returns the natural logarithm (base e) of x.");

static PyObject *
math_log10(PyObject *self, PyObject *arg)
{
	return loghelper(arg, log10, "log10");
}

PyDoc_STRVAR(math_log10_doc,
"log10(x) -> the base 10 logarithm of x.");

static PyObject *
math_fmod(PyObject *self, PyObject *args)
{
	PyObject *ox, *oy;
	double r, x, y;
	if (! PyArg_UnpackTuple(args, "fmod", 2, 2, &ox, &oy))
		return NULL;
	x = PyFloat_AsDouble(ox);
	y = PyFloat_AsDouble(oy);
	if ((x == -1.0 || y == -1.0) && PyErr_Occurred())
		return NULL;
	/* fmod(x, +/-Inf) returns x for finite x. */
	if (Py_IS_INFINITY(y) && Py_IS_FINITE(x))
		return PyFloat_FromDouble(x);
	errno = 0;
	PyFPE_START_PROTECT("in math_fmod", return 0);
	r = fmod(x, y);
	PyFPE_END_PROTECT(r);
	if (Py_IS_NAN(r)) {
		if (!Py_IS_NAN(x) && !Py_IS_NAN(y))
			errno = EDOM;
		else
			errno = 0;
	}
	if (errno && is_error(r))
		return NULL;
	else
		return PyFloat_FromDouble(r);
}

PyDoc_STRVAR(math_fmod_doc,
"fmod(x,y)\n\nReturn fmod(x, y), according to platform C."
"  x % y may differ.");

static PyObject *
math_hypot(PyObject *self, PyObject *args)
{
	PyObject *ox, *oy;
	double r, x, y;
	if (! PyArg_UnpackTuple(args, "hypot", 2, 2, &ox, &oy))
		return NULL;
	x = PyFloat_AsDouble(ox);
	y = PyFloat_AsDouble(oy);
	if ((x == -1.0 || y == -1.0) && PyErr_Occurred())
		return NULL;
	/* hypot(x, +/-Inf) returns Inf, even if x is a NaN. */
	if (Py_IS_INFINITY(x))
		return PyFloat_FromDouble(fabs(x));
	if (Py_IS_INFINITY(y))
		return PyFloat_FromDouble(fabs(y));
	errno = 0;
	PyFPE_START_PROTECT("in math_hypot", return 0);
	r = hypot(x, y);
	PyFPE_END_PROTECT(r);
	if (Py_IS_NAN(r)) {
		if (!Py_IS_NAN(x) && !Py_IS_NAN(y))
			errno = EDOM;
		else
			errno = 0;
	}
	else if (Py_IS_INFINITY(r)) {
		if (Py_IS_FINITE(x) && Py_IS_FINITE(y))
			errno = ERANGE;
		else
			errno = 0;
	}
	if (errno && is_error(r))
		return NULL;
	else
		return PyFloat_FromDouble(r);
}

PyDoc_STRVAR(math_hypot_doc,
"hypot(x,y)\n\nReturn the Euclidean distance, sqrt(x*x + y*y).");

/* pow can't use math_2, but needs its own wrapper: the problem is
   that an infinite result can arise either as a result of overflow
   (in which case OverflowError should be raised) or as a result of
   e.g. 0.**-5. (for which ValueError needs to be raised.)
*/

static PyObject *
math_pow(PyObject *self, PyObject *args)
{
	PyObject *ox, *oy;
	double r, x, y;
	int odd_y;

	if (! PyArg_UnpackTuple(args, "pow", 2, 2, &ox, &oy))
		return NULL;
	x = PyFloat_AsDouble(ox);
	y = PyFloat_AsDouble(oy);
	if ((x == -1.0 || y == -1.0) && PyErr_Occurred())
		return NULL;

	/* deal directly with IEEE specials, to cope with problems on various
	   platforms whose semantics don't exactly match C99 */
	r = 0.; /* silence compiler warning */
	if (!Py_IS_FINITE(x) || !Py_IS_FINITE(y)) {
		errno = 0;
		if (Py_IS_NAN(x))
			r = y == 0. ? 1. : x; /* NaN**0 = 1 */
		else if (Py_IS_NAN(y))
			r = x == 1. ? 1. : y; /* 1**NaN = 1 */
		else if (Py_IS_INFINITY(x)) {
			odd_y = Py_IS_FINITE(y) && fmod(fabs(y), 2.0) == 1.0;
			if (y > 0.)
				r = odd_y ? x : fabs(x);
			else if (y == 0.)
				r = 1.;
			else /* y < 0. */
				r = odd_y ? copysign(0., x) : 0.;
		}
		else if (Py_IS_INFINITY(y)) {
			if (fabs(x) == 1.0)
				r = 1.;
			else if (y > 0. && fabs(x) > 1.0)
				r = y;
			else if (y < 0. && fabs(x) < 1.0) {
				r = -y; /* result is +inf */
				if (x == 0.) /* 0**-inf: divide-by-zero */
					errno = EDOM;
			}
			else
				r = 0.;
		}
	}
	else {
		/* let libm handle finite**finite */
		errno = 0;
		PyFPE_START_PROTECT("in math_pow", return 0);
		r = pow(x, y);
		PyFPE_END_PROTECT(r);
		/* a NaN result should arise only from (-ve)**(finite
		   non-integer); in this case we want to raise ValueError. */
		if (!Py_IS_FINITE(r)) {
			if (Py_IS_NAN(r)) {
				errno = EDOM;
			}
			/* 
			   an infinite result here arises either from:
			   (A) (+/-0.)**negative (-> divide-by-zero)
			   (B) overflow of x**y with x and y finite
			*/
			else if (Py_IS_INFINITY(r)) {
				if (x == 0.)
					errno = EDOM;
				else
					errno = ERANGE;
			}
		}
	}

	if (errno && is_error(r))
		return NULL;
	else
		return PyFloat_FromDouble(r);
}

PyDoc_STRVAR(math_pow_doc,
"pow(x,y)\n\nReturn x**y (x to the power of y).");

static const double degToRad = Py_MATH_PI / 180.0;
static const double radToDeg = 180.0 / Py_MATH_PI;

static PyObject *
math_degrees(PyObject *self, PyObject *arg)
{
	double x = PyFloat_AsDouble(arg);
	if (x == -1.0 && PyErr_Occurred())
		return NULL;
	return PyFloat_FromDouble(x * radToDeg);
}

PyDoc_STRVAR(math_degrees_doc,
"degrees(x) -> converts angle x from radians to degrees");

static PyObject *
math_radians(PyObject *self, PyObject *arg)
{
	double x = PyFloat_AsDouble(arg);
	if (x == -1.0 && PyErr_Occurred())
		return NULL;
	return PyFloat_FromDouble(x * degToRad);
}

PyDoc_STRVAR(math_radians_doc,
"radians(x) -> converts angle x from degrees to radians");

static PyObject *
math_isnan(PyObject *self, PyObject *arg)
{
	double x = PyFloat_AsDouble(arg);
	if (x == -1.0 && PyErr_Occurred())
		return NULL;
	return PyBool_FromLong((long)Py_IS_NAN(x));
}

PyDoc_STRVAR(math_isnan_doc,
"isnan(x) -> bool\n\
Checks if float x is not a number (NaN)");

static PyObject *
math_isinf(PyObject *self, PyObject *arg)
{
	double x = PyFloat_AsDouble(arg);
	if (x == -1.0 && PyErr_Occurred())
		return NULL;
	return PyBool_FromLong((long)Py_IS_INFINITY(x));
}

PyDoc_STRVAR(math_isinf_doc,
"isinf(x) -> bool\n\
Checks if float x is infinite (positive or negative)");

static PyMethodDef math_methods[] = {
	{"acos",	math_acos,	METH_O,		math_acos_doc},
	{"acosh",	math_acosh,	METH_O,		math_acosh_doc},
	{"asin",	math_asin,	METH_O,		math_asin_doc},
	{"asinh",	math_asinh,	METH_O,		math_asinh_doc},
	{"atan",	math_atan,	METH_O,		math_atan_doc},
	{"atan2",	math_atan2,	METH_VARARGS,	math_atan2_doc},
	{"atanh",	math_atanh,	METH_O,		math_atanh_doc},
	{"ceil",	math_ceil,	METH_O,		math_ceil_doc},
	{"copysign",	math_copysign,	METH_VARARGS,	math_copysign_doc},
	{"cos",		math_cos,	METH_O,		math_cos_doc},
	{"cosh",	math_cosh,	METH_O,		math_cosh_doc},
	{"degrees",	math_degrees,	METH_O,		math_degrees_doc},
	{"exp",		math_exp,	METH_O,		math_exp_doc},
	{"fabs",	math_fabs,	METH_O,		math_fabs_doc},
	{"floor",	math_floor,	METH_O,		math_floor_doc},
	{"fmod",	math_fmod,	METH_VARARGS,	math_fmod_doc},
	{"frexp",	math_frexp,	METH_O,		math_frexp_doc},
	{"hypot",	math_hypot,	METH_VARARGS,	math_hypot_doc},
	{"isinf",	math_isinf,	METH_O,		math_isinf_doc},
	{"isnan",	math_isnan,	METH_O,		math_isnan_doc},
	{"ldexp",	math_ldexp,	METH_VARARGS,	math_ldexp_doc},
	{"log",		math_log,	METH_VARARGS,	math_log_doc},
	{"log1p",	math_log1p,	METH_O,		math_log1p_doc},
	{"log10",	math_log10,	METH_O,		math_log10_doc},
	{"modf",	math_modf,	METH_O,		math_modf_doc},
	{"pow",		math_pow,	METH_VARARGS,	math_pow_doc},
	{"radians",	math_radians,	METH_O,		math_radians_doc},
	{"sin",		math_sin,	METH_O,		math_sin_doc},
	{"sinh",	math_sinh,	METH_O,		math_sinh_doc},
	{"sqrt",	math_sqrt,	METH_O,		math_sqrt_doc},
	{"tan",		math_tan,	METH_O,		math_tan_doc},
	{"tanh",	math_tanh,	METH_O,		math_tanh_doc},
 	{"trunc",	math_trunc,	METH_O,		math_trunc_doc},
	{NULL,		NULL}		/* sentinel */
};


PyDoc_STRVAR(module_doc,
"This module is always available.  It provides access to the\n"
"mathematical functions defined by the C standard.");

PyMODINIT_FUNC
initmath(void)
{
	PyObject *m;

	m = Py_InitModule3("math", math_methods, module_doc);
	if (m == NULL)
		goto finally;

	PyModule_AddObject(m, "pi", PyFloat_FromDouble(Py_MATH_PI));
	PyModule_AddObject(m, "e", PyFloat_FromDouble(Py_MATH_E));

    finally:
	return;
}
