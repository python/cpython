/* Math module -- standard C math library functions, pi and e */

#include "Python.h"
#include "longintrepr.h" /* just for SHIFT */

#ifndef _MSC_VER
#ifndef __STDC__
extern double fmod (double, double);
extern double frexp (double, int *);
extern double ldexp (double, int);
extern double modf (double, double *);
#endif /* __STDC__ */
#endif /* _MSC_VER */

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

static PyObject *
math_1_to_whatever(PyObject *arg, double (*func) (double),
                   PyObject *(*from_double_func) (double))
{
	double x = PyFloat_AsDouble(arg);
	if (x == -1.0 && PyErr_Occurred())
		return NULL;
	errno = 0;
	PyFPE_START_PROTECT("in math_1", return 0)
	x = (*func)(x);
	PyFPE_END_PROTECT(x)
	Py_SET_ERRNO_ON_MATH_ERROR(x);
	if (errno && is_error(x))
		return NULL;
	else
        	return (*from_double_func)(x);
}

static PyObject *
math_1(PyObject *arg, double (*func) (double))
{
	return math_1_to_whatever(arg, func, PyFloat_FromDouble);
}

static PyObject *
math_1_to_int(PyObject *arg, double (*func) (double))
{
	return math_1_to_whatever(arg, func, PyLong_FromDouble);
}

static PyObject *
math_2(PyObject *args, double (*func) (double, double), char *funcname)
{
	PyObject *ox, *oy;
	double x, y;
	if (! PyArg_UnpackTuple(args, funcname, 2, 2, &ox, &oy))
		return NULL;
	x = PyFloat_AsDouble(ox);
	y = PyFloat_AsDouble(oy);
	if ((x == -1.0 || y == -1.0) && PyErr_Occurred())
		return NULL;
	errno = 0;
	PyFPE_START_PROTECT("in math_2", return 0)
	x = (*func)(x, y);
	PyFPE_END_PROTECT(x)
	Py_SET_ERRNO_ON_MATH_ERROR(x);
	if (errno && is_error(x))
		return NULL;
	else
		return PyFloat_FromDouble(x);
}

#define FUNC1(funcname, func, docstring) \
	static PyObject * math_##funcname(PyObject *self, PyObject *args) { \
		return math_1(args, func); \
	}\
        PyDoc_STRVAR(math_##funcname##_doc, docstring);

#define FUNC2(funcname, func, docstring) \
	static PyObject * math_##funcname(PyObject *self, PyObject *args) { \
		return math_2(args, func, #funcname); \
	}\
        PyDoc_STRVAR(math_##funcname##_doc, docstring);

FUNC1(acos, acos,
      "acos(x)\n\nReturn the arc cosine (measured in radians) of x.")
FUNC1(asin, asin,
      "asin(x)\n\nReturn the arc sine (measured in radians) of x.")
FUNC1(atan, atan,
      "atan(x)\n\nReturn the arc tangent (measured in radians) of x.")
FUNC2(atan2, atan2,
      "atan2(y, x)\n\nReturn the arc tangent (measured in radians) of y/x.\n"
      "Unlike atan(y/x), the signs of both x and y are considered.")

static PyObject * math_ceil(PyObject *self, PyObject *number) {
	static PyObject *ceil_str = NULL;
	PyObject *method;

	if (ceil_str == NULL) {
		ceil_str = PyUnicode_FromString("__ceil__");
		if (ceil_str == NULL)
			return NULL;
	}

	method = _PyType_Lookup(Py_TYPE(number), ceil_str);
	if (method == NULL)
		return math_1_to_int(number, ceil);
	else
		return PyObject_CallFunction(method, "O", number);
}

PyDoc_STRVAR(math_ceil_doc,
	     "ceil(x)\n\nReturn the ceiling of x as an int.\n"
	     "This is the smallest integral value >= x.");

FUNC1(cos, cos,
      "cos(x)\n\nReturn the cosine of x (measured in radians).")
FUNC1(cosh, cosh,
      "cosh(x)\n\nReturn the hyperbolic cosine of x.")

#ifdef MS_WINDOWS
#  define copysign _copysign
#  define HAVE_COPYSIGN 1
#endif
#ifdef HAVE_COPYSIGN
FUNC2(copysign, copysign,
      "copysign(x,y)\n\nReturn x with the sign of y.");
#endif

FUNC1(exp, exp,
      "exp(x)\n\nReturn e raised to the power of x.")
FUNC1(fabs, fabs,
      "fabs(x)\n\nReturn the absolute value of the float x.")

static PyObject * math_floor(PyObject *self, PyObject *number) {
	static PyObject *floor_str = NULL;
	PyObject *method;

	if (floor_str == NULL) {
		floor_str = PyUnicode_FromString("__floor__");
		if (floor_str == NULL)
			return NULL;
	}

	method = _PyType_Lookup(Py_TYPE(number), floor_str);
	if (method == NULL)
        	return math_1_to_int(number, floor);
	else
		return PyObject_CallFunction(method, "O", number);
}

PyDoc_STRVAR(math_floor_doc,
	     "floor(x)\n\nReturn the floor of x as an int.\n"
	     "This is the largest integral value <= x.");

FUNC2(fmod, fmod,
      "fmod(x,y)\n\nReturn fmod(x, y), according to platform C."
      "  x % y may differ.")
FUNC2(hypot, hypot,
      "hypot(x,y)\n\nReturn the Euclidean distance, sqrt(x*x + y*y).")
FUNC2(pow, pow,
      "pow(x,y)\n\nReturn x**y (x to the power of y).")
FUNC1(sin, sin,
      "sin(x)\n\nReturn the sine of x (measured in radians).")
FUNC1(sinh, sinh,
      "sinh(x)\n\nReturn the hyperbolic sine of x.")
FUNC1(sqrt, sqrt,
      "sqrt(x)\n\nReturn the square root of x.")
FUNC1(tan, tan,
      "tan(x)\n\nReturn the tangent of x (measured in radians).")
FUNC1(tanh, tanh,
      "tanh(x)\n\nReturn the hyperbolic tangent of x.")

static PyObject *
math_frexp(PyObject *self, PyObject *arg)
{
	int i;
	double x = PyFloat_AsDouble(arg);
	if (x == -1.0 && PyErr_Occurred())
		return NULL;
	errno = 0;
	x = frexp(x, &i);
	Py_SET_ERRNO_ON_MATH_ERROR(x);
	if (errno && is_error(x))
		return NULL;
	else
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
	double x;
	int exp;
	if (! PyArg_ParseTuple(args, "di:ldexp", &x, &exp))
		return NULL;
	errno = 0;
	PyFPE_START_PROTECT("ldexp", return 0)
	x = ldexp(x, exp);
	PyFPE_END_PROTECT(x)
	Py_SET_ERRNO_ON_MATH_ERROR(x);
	if (errno && is_error(x))
		return NULL;
	else
		return PyFloat_FromDouble(x);
}

PyDoc_STRVAR(math_ldexp_doc,
"ldexp(x, i) -> x * (2**i)");

static PyObject *
math_modf(PyObject *self, PyObject *arg)
{
	double y, x = PyFloat_AsDouble(arg);
	if (x == -1.0 && PyErr_Occurred())
		return NULL;
	errno = 0;
	x = modf(x, &y);
	Py_SET_ERRNO_ON_MATH_ERROR(x);
	if (errno && is_error(x))
		return NULL;
	else
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
		/* Value is ~= x * 2**(e*SHIFT), so the log ~=
		   log(x) + log(2) * e * SHIFT.
		   CAUTION:  e*SHIFT may overflow using int arithmetic,
		   so force use of double. */
		x = func(x) + (e * (double)PyLong_SHIFT) * func(2.0);
		return PyFloat_FromDouble(x);
	}

	/* Else let libm handle it by itself. */
	return math_1(arg, func);
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
	{"asin",	math_asin,	METH_O,		math_asin_doc},
	{"atan",	math_atan,	METH_O,		math_atan_doc},
	{"atan2",	math_atan2,	METH_VARARGS,	math_atan2_doc},
	{"ceil",	math_ceil,	METH_O,		math_ceil_doc},
#ifdef HAVE_COPYSIGN
	{"copysign",	math_copysign,	METH_VARARGS,	math_copysign_doc},
#endif
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
	{"log10",	math_log10,	METH_O,		math_log10_doc},
	{"modf",	math_modf,	METH_O,		math_modf_doc},
	{"pow",		math_pow,	METH_VARARGS,	math_pow_doc},
	{"radians",	math_radians,	METH_O,		math_radians_doc},
	{"sin",		math_sin,	METH_O,		math_sin_doc},
	{"sinh",	math_sinh,	METH_O,		math_sinh_doc},
	{"sqrt",	math_sqrt,	METH_O,		math_sqrt_doc},
	{"tan",		math_tan,	METH_O,		math_tan_doc},
	{"tanh",	math_tanh,	METH_O,		math_tanh_doc},
	{NULL,		NULL}		/* sentinel */
};


PyDoc_STRVAR(module_doc,
"This module is always available.  It provides access to the\n"
"mathematical functions defined by the C standard.");

PyMODINIT_FUNC
initmath(void)
{
	PyObject *m, *d, *v;

	m = Py_InitModule3("math", math_methods, module_doc);
	if (m == NULL)
		goto finally;
	d = PyModule_GetDict(m);
	if (d == NULL)
		goto finally;

        if (!(v = PyFloat_FromDouble(Py_MATH_PI)))
                goto finally;
	if (PyDict_SetItemString(d, "pi", v) < 0)
                goto finally;
	Py_DECREF(v);

        if (!(v = PyFloat_FromDouble(Py_MATH_E)))
                goto finally;
	if (PyDict_SetItemString(d, "e", v) < 0)
                goto finally;
	Py_DECREF(v);

  finally:
	return;
}
