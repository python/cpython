/* Math module -- standard C math library functions, pi and e */

#include "Python.h"

#ifndef _MSC_VER
#ifndef __STDC__
extern double fmod (double, double);
extern double frexp (double, int *);
extern double ldexp (double, int);
extern double modf (double, double *);
#endif /* __STDC__ */
#endif /* _MSC_VER */


#ifdef i860
/* Cray APP has bogus definition of HUGE_VAL in <math.h> */
#undef HUGE_VAL
#endif

/* RED_FLAG 12-Oct-2000 Tim
 * What CHECK does if errno == 0 and x is a NaN is a platform-dependent crap
 * shoot.  Most (but not all!) platforms will end up setting errno to ERANGE
 * then, but EDOM is probably better.
 */
#ifdef HUGE_VAL
#define CHECK(x) if (errno != 0) ; \
	else if (-HUGE_VAL <= (x) && (x) <= HUGE_VAL) ; \
	else errno = ERANGE
#else
#define CHECK(x) /* Don't know how to check */
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
		 * the latter across platforms.  Here we suppress the
		 * underflow errors (libm functions should return a zero
		 * on underflow, and +- HUGE_VAL on overflow, so testing
		 * the result for zero suffices to distinguish the cases).
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
math_1(PyObject *args, double (*func) (double), char *argsfmt)
{
	double x;
	if (!  PyArg_ParseTuple(args, argsfmt, &x))
		return NULL;
	errno = 0;
	PyFPE_START_PROTECT("in math_1", return 0)
	x = (*func)(x);
	PyFPE_END_PROTECT(x)
	CHECK(x);
	if (errno && is_error(x))
		return NULL;
	else
		return PyFloat_FromDouble(x);
}

static PyObject *
math_2(PyObject *args, double (*func) (double, double), char *argsfmt)
{
	double x, y;
	if (! PyArg_ParseTuple(args, argsfmt, &x, &y))
		return NULL;
	errno = 0;
	PyFPE_START_PROTECT("in math_2", return 0)
	x = (*func)(x, y);
	PyFPE_END_PROTECT(x)
	CHECK(x);
	if (errno && is_error(x))
		return NULL;
	else
		return PyFloat_FromDouble(x);
}

#define FUNC1(funcname, func, docstring) \
	static PyObject * math_##funcname(PyObject *self, PyObject *args) { \
		return math_1(args, func, "d:" #funcname); \
	}\
        static char math_##funcname##_doc [] = docstring;

#define FUNC2(funcname, func, docstring) \
	static PyObject * math_##funcname(PyObject *self, PyObject *args) { \
		return math_2(args, func, "dd:" #funcname); \
	}\
        static char math_##funcname##_doc [] = docstring;

FUNC1(acos, acos,
      "acos(x)\n\nReturn the arc cosine of x.")
FUNC1(asin, asin,
      "asin(x)\n\nReturn the arc sine of x.")
FUNC1(atan, atan,
      "atan(x)\n\nReturn the arc tangent of x.")
FUNC2(atan2, atan2,
      "atan2(y, x)\n\nReturn atan(y/x).")
FUNC1(ceil, ceil,
      "ceil(x)\n\nReturn the ceiling of x as a real.")
FUNC1(cos, cos,
      "cos(x)\n\nReturn the cosine of x.")
FUNC1(cosh, cosh,
      "cosh(x)\n\nReturn the hyperbolic cosine of x.")
FUNC1(exp, exp,
      "exp(x)\n\nReturn e raised to the power of x.")
FUNC1(fabs, fabs,
      "fabs(x)\n\nReturn the absolute value of the real x.")
FUNC1(floor, floor,
      "floor(x)\n\nReturn the floor of x as a real.")
     FUNC2(fmod, fmod,
      "fmod(x,y)\n\nReturn fmod(x, y), according to platform C."
      "  x % y may differ.")
FUNC2(hypot, hypot,
      "hypot(x,y)\n\nReturn the Euclidean distance, sqrt(x*x + y*y).")
FUNC1(log, log,
      "log(x)\n\nReturn the natural logarithm of x.")
FUNC1(log10, log10,
      "log10(x)\n\nReturn the base-10 logarithm of x.")
#ifdef MPW_3_1 /* This hack is needed for MPW 3.1 but not for 3.2 ... */
FUNC2(pow, power,
      "pow(x,y)\n\nReturn x**y.")
#else
FUNC2(pow, pow,
      "pow(x,y)\n\nReturn x**y.")
#endif
FUNC1(sin, sin,
      "sin(x)\n\nReturn the sine of x.")
FUNC1(sinh, sinh,
      "sinh(x)\n\nReturn the hyperbolic sine of x.")
FUNC1(sqrt, sqrt,
      "sqrt(x)\n\nReturn the square root of x.")
FUNC1(tan, tan,
      "tan(x)\n\nReturn the tangent of x.")
FUNC1(tanh, tanh,
      "tanh(x)\n\nReturn the hyperbolic tangent of x.")


static PyObject *
math_frexp(PyObject *self, PyObject *args)
{
	double x;
	int i;
	if (! PyArg_ParseTuple(args, "d:frexp", &x))
		return NULL;
	errno = 0;
	x = frexp(x, &i);
	CHECK(x);
	if (errno && is_error(x))
		return NULL;
	else
		return Py_BuildValue("(di)", x, i);
}

static char math_frexp_doc [] =
"frexp(x)\n\
\n\
Return the mantissa and exponent of x, as pair (m, e).\n\
m is a float and e is an int, such that x = m * 2.**e.\n\
If x is 0, m and e are both 0.  Else 0.5 <= abs(m) < 1.0.";


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
	CHECK(x);
	if (errno && is_error(x))
		return NULL;
	else
		return PyFloat_FromDouble(x);
}

static char math_ldexp_doc [] = 
"ldexp_doc(x, i)\n\
\n\
Return x * (2**i).";


static PyObject *
math_modf(PyObject *self, PyObject *args)
{
	double x, y;
	if (! PyArg_ParseTuple(args, "d:modf", &x))
		return NULL;
	errno = 0;
#ifdef MPW /* MPW C modf expects pointer to extended as second argument */
        {
		extended e;
		x = modf(x, &e);
		y = e;
        }
#else
	x = modf(x, &y);
#endif
	CHECK(x);
	if (errno && is_error(x))
		return NULL;
	else
		return Py_BuildValue("(dd)", x, y);
}

static char math_modf_doc [] =
"modf(x)\n\
\n\
Return the fractional and integer parts of x. Both results carry the sign\n\
of x.  The integer part is returned as a real.";


static PyMethodDef math_methods[] = {
	{"acos",	math_acos,	METH_VARARGS,	math_acos_doc},
	{"asin",	math_asin,	METH_VARARGS,	math_asin_doc},
	{"atan",	math_atan,	METH_VARARGS,	math_atan_doc},
	{"atan2",	math_atan2,	METH_VARARGS,	math_atan2_doc},
	{"ceil",	math_ceil,	METH_VARARGS,	math_ceil_doc},
	{"cos",		math_cos,	METH_VARARGS,	math_cos_doc},
	{"cosh",	math_cosh,	METH_VARARGS,	math_cosh_doc},
	{"exp",		math_exp,	METH_VARARGS,	math_exp_doc},
	{"fabs",	math_fabs,	METH_VARARGS,	math_fabs_doc},
	{"floor",	math_floor,	METH_VARARGS,	math_floor_doc},
	{"fmod",	math_fmod,	METH_VARARGS,	math_fmod_doc},
	{"frexp",	math_frexp,	METH_VARARGS,	math_frexp_doc},
	{"hypot",	math_hypot,	METH_VARARGS,	math_hypot_doc},
	{"ldexp",	math_ldexp,	METH_VARARGS,	math_ldexp_doc},
	{"log",		math_log,	METH_VARARGS,	math_log_doc},
	{"log10",	math_log10,	METH_VARARGS,	math_log10_doc},
	{"modf",	math_modf,	METH_VARARGS,	math_modf_doc},
	{"pow",		math_pow,	METH_VARARGS,	math_pow_doc},
	{"sin",		math_sin,	METH_VARARGS,	math_sin_doc},
	{"sinh",	math_sinh,	METH_VARARGS,	math_sinh_doc},
	{"sqrt",	math_sqrt,	METH_VARARGS,	math_sqrt_doc},
	{"tan",		math_tan,	METH_VARARGS,	math_tan_doc},
	{"tanh",	math_tanh,	METH_VARARGS,	math_tanh_doc},
	{NULL,		NULL}		/* sentinel */
};


static char module_doc [] =
"This module is always available.  It provides access to the\n\
mathematical functions defined by the C standard.";

DL_EXPORT(void)
initmath(void)
{
	PyObject *m, *d, *v;
	
	m = Py_InitModule3("math", math_methods, module_doc);
	d = PyModule_GetDict(m);

        if (!(v = PyFloat_FromDouble(atan(1.0) * 4.0)))
                goto finally;
	if (PyDict_SetItemString(d, "pi", v) < 0)
                goto finally;
	Py_DECREF(v);

        if (!(v = PyFloat_FromDouble(exp(1.0))))
                goto finally;
	if (PyDict_SetItemString(d, "e", v) < 0)
                goto finally;
	Py_DECREF(v);

  finally:
	return;
}
