/* Complex object implementation */

/* Borrows heavily from floatobject.c */

/* Submitted by Jim Hugunin */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_complexobject.h" // _PyComplex_FormatAdvancedWriter()
#include "pycore_floatobject.h"   // _Py_convert_int_to_double()
#include "pycore_freelist.h"      // _Py_FREELIST_FREE(), _Py_FREELIST_POP()
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_object.h"        // _PyObject_Init()
#include "pycore_pymath.h"        // _Py_ADJUST_ERANGE2()


#define _PyComplexObject_CAST(op)   ((PyComplexObject *)(op))


/*[clinic input]
class complex "PyComplexObject *" "&PyComplex_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=819e057d2d10f5ec]*/

#include "clinic/complexobject.c.h"

/* elementary operations on complex numbers */

static Py_complex c_1 = {1., 0.};

Py_complex
_Py_c_sum(Py_complex a, Py_complex b)
{
    Py_complex r;
    r.real = a.real + b.real;
    r.imag = a.imag + b.imag;
    return r;
}

Py_complex
_Py_cr_sum(Py_complex a, double b)
{
    Py_complex r = a;
    r.real += b;
    return r;
}

static inline Py_complex
_Py_rc_sum(double a, Py_complex b)
{
    return _Py_cr_sum(b, a);
}

Py_complex
_Py_c_diff(Py_complex a, Py_complex b)
{
    Py_complex r;
    r.real = a.real - b.real;
    r.imag = a.imag - b.imag;
    return r;
}

Py_complex
_Py_cr_diff(Py_complex a, double b)
{
    Py_complex r = a;
    r.real -= b;
    return r;
}

Py_complex
_Py_rc_diff(double a, Py_complex b)
{
    Py_complex r;
    r.real = a - b.real;
    r.imag = -b.imag;
    return r;
}

Py_complex
_Py_c_neg(Py_complex a)
{
    Py_complex r;
    r.real = -a.real;
    r.imag = -a.imag;
    return r;
}

Py_complex
_Py_c_prod(Py_complex z, Py_complex w)
{
    double a = z.real, b = z.imag, c = w.real, d = w.imag;
    double ac = a*c, bd = b*d, ad = a*d, bc = b*c;
    Py_complex r = {ac - bd, ad + bc};

    /* Recover infinities that computed as nan+nanj.  See e.g. the C11,
       Annex G.5.1, routine _Cmultd(). */
    if (isnan(r.real) && isnan(r.imag)) {
        int recalc = 0;

        if (isinf(a) || isinf(b)) {  /* z is infinite */
            /* "Box" the infinity and change nans in the other factor to 0 */
            a = copysign(isinf(a) ? 1.0 : 0.0, a);
            b = copysign(isinf(b) ? 1.0 : 0.0, b);
            if (isnan(c)) {
                c = copysign(0.0, c);
            }
            if (isnan(d)) {
                d = copysign(0.0, d);
            }
            recalc = 1;
        }
        if (isinf(c) || isinf(d)) {  /* w is infinite */
            /* "Box" the infinity and change nans in the other factor to 0 */
            c = copysign(isinf(c) ? 1.0 : 0.0, c);
            d = copysign(isinf(d) ? 1.0 : 0.0, d);
            if (isnan(a)) {
                a = copysign(0.0, a);
            }
            if (isnan(b)) {
                b = copysign(0.0, b);
            }
            recalc = 1;
        }
        if (!recalc && (isinf(ac) || isinf(bd) || isinf(ad) || isinf(bc))) {
            /* Recover infinities from overflow by changing nans to 0 */
            if (isnan(a)) {
                a = copysign(0.0, a);
            }
            if (isnan(b)) {
                b = copysign(0.0, b);
            }
            if (isnan(c)) {
                c = copysign(0.0, c);
            }
            if (isnan(d)) {
                d = copysign(0.0, d);
            }
            recalc = 1;
        }
        if (recalc) {
            r.real = Py_INFINITY*(a*c - b*d);
            r.imag = Py_INFINITY*(a*d + b*c);
        }
    }

    return r;
}

Py_complex
_Py_cr_prod(Py_complex a, double b)
{
    Py_complex r = a;
    r.real *= b;
    r.imag *= b;
    return r;
}

static inline Py_complex
_Py_rc_prod(double a, Py_complex b)
{
    return _Py_cr_prod(b, a);
}

/* Avoid bad optimization on Windows ARM64 until the compiler is fixed */
#ifdef _M_ARM64
#pragma optimize("", off)
#endif
Py_complex
_Py_c_quot(Py_complex a, Py_complex b)
{
    /******************************************************************
    This was the original algorithm.  It's grossly prone to spurious
    overflow and underflow errors.  It also merrily divides by 0 despite
    checking for that(!).  The code still serves a doc purpose here, as
    the algorithm following is a simple by-cases transformation of this
    one:

    Py_complex r;
    double d = b.real*b.real + b.imag*b.imag;
    if (d == 0.)
        errno = EDOM;
    r.real = (a.real*b.real + a.imag*b.imag)/d;
    r.imag = (a.imag*b.real - a.real*b.imag)/d;
    return r;
    ******************************************************************/

    /* This algorithm is better, and is pretty obvious:  first divide the
     * numerators and denominator by whichever of {b.real, b.imag} has
     * larger magnitude.  The earliest reference I found was to CACM
     * Algorithm 116 (Complex Division, Robert L. Smith, Stanford
     * University).
     */
     Py_complex r;      /* the result */
     const double abs_breal = b.real < 0 ? -b.real : b.real;
     const double abs_bimag = b.imag < 0 ? -b.imag : b.imag;

    if (abs_breal >= abs_bimag) {
        /* divide tops and bottom by b.real */
        if (abs_breal == 0.0) {
            errno = EDOM;
            r.real = r.imag = 0.0;
        }
        else {
            const double ratio = b.imag / b.real;
            const double denom = b.real + b.imag * ratio;
            r.real = (a.real + a.imag * ratio) / denom;
            r.imag = (a.imag - a.real * ratio) / denom;
        }
    }
    else if (abs_bimag >= abs_breal) {
        /* divide tops and bottom by b.imag */
        const double ratio = b.real / b.imag;
        const double denom = b.real * ratio + b.imag;
        assert(b.imag != 0.0);
        r.real = (a.real * ratio + a.imag) / denom;
        r.imag = (a.imag * ratio - a.real) / denom;
    }
    else {
        /* At least one of b.real or b.imag is a NaN */
        r.real = r.imag = Py_NAN;
    }

    /* Recover infinities and zeros that computed as nan+nanj.  See e.g.
       the C11, Annex G.5.2, routine _Cdivd(). */
    if (isnan(r.real) && isnan(r.imag)) {
        if ((isinf(a.real) || isinf(a.imag))
            && isfinite(b.real) && isfinite(b.imag))
        {
            const double x = copysign(isinf(a.real) ? 1.0 : 0.0, a.real);
            const double y = copysign(isinf(a.imag) ? 1.0 : 0.0, a.imag);
            r.real = Py_INFINITY * (x*b.real + y*b.imag);
            r.imag = Py_INFINITY * (y*b.real - x*b.imag);
        }
        else if ((isinf(abs_breal) || isinf(abs_bimag))
                 && isfinite(a.real) && isfinite(a.imag))
        {
            const double x = copysign(isinf(b.real) ? 1.0 : 0.0, b.real);
            const double y = copysign(isinf(b.imag) ? 1.0 : 0.0, b.imag);
            r.real = 0.0 * (a.real*x + a.imag*y);
            r.imag = 0.0 * (a.imag*x - a.real*y);
        }
    }

    return r;
}

Py_complex
_Py_cr_quot(Py_complex a, double b)
{
    Py_complex r = a;
    if (b) {
        r.real /= b;
        r.imag /= b;
    }
    else {
        errno = EDOM;
        r.real = r.imag = 0.0;
    }
    return r;
}

/* an equivalent of _Py_c_quot() function, when 1st argument is real */
Py_complex
_Py_rc_quot(double a, Py_complex b)
{
    Py_complex r;
    const double abs_breal = b.real < 0 ? -b.real : b.real;
    const double abs_bimag = b.imag < 0 ? -b.imag : b.imag;

    if (abs_breal >= abs_bimag) {
        if (abs_breal == 0.0) {
            errno = EDOM;
            r.real = r.imag = 0.0;
        }
        else {
            const double ratio = b.imag / b.real;
            const double denom = b.real + b.imag * ratio;
            r.real = a / denom;
            r.imag = (-a * ratio) / denom;
        }
    }
    else if (abs_bimag >= abs_breal) {
        const double ratio = b.real / b.imag;
        const double denom = b.real * ratio + b.imag;
        assert(b.imag != 0.0);
        r.real = (a * ratio) / denom;
        r.imag = (-a) / denom;
    }
    else {
        r.real = r.imag = Py_NAN;
    }

    if (isnan(r.real) && isnan(r.imag) && isfinite(a)
        && (isinf(abs_breal) || isinf(abs_bimag)))
    {
        const double x = copysign(isinf(b.real) ? 1.0 : 0.0, b.real);
        const double y = copysign(isinf(b.imag) ? 1.0 : 0.0, b.imag);
        r.real = 0.0 * (a*x);
        r.imag = 0.0 * (-a*y);
    }

    return r;
}
#ifdef _M_ARM64
#pragma optimize("", on)
#endif

Py_complex
_Py_c_pow(Py_complex a, Py_complex b)
{
    Py_complex r;
    double vabs,len,at,phase;
    if (b.real == 0. && b.imag == 0.) {
        r.real = 1.;
        r.imag = 0.;
    }
    else if (a.real == 0. && a.imag == 0.) {
        if (b.imag != 0. || b.real < 0.)
            errno = EDOM;
        r.real = 0.;
        r.imag = 0.;
    }
    else {
        vabs = hypot(a.real,a.imag);
        len = pow(vabs,b.real);
        at = atan2(a.imag, a.real);
        phase = at*b.real;
        if (b.imag != 0.0) {
            len *= exp(-at*b.imag);
            phase += b.imag*log(vabs);
        }
        r.real = len*cos(phase);
        r.imag = len*sin(phase);

        _Py_ADJUST_ERANGE2(r.real, r.imag);
    }
    return r;
}

static Py_complex
c_powu(Py_complex x, long n)
{
    Py_complex r, p;
    long mask = 1;
    r = c_1;
    p = x;
    while (mask > 0 && n >= mask) {
        if (n & mask)
            r = _Py_c_prod(r,p);
        mask <<= 1;
        p = _Py_c_prod(p,p);
    }
    return r;
}

static Py_complex
c_powi(Py_complex x, long n)
{
    if (n > 0)
        return c_powu(x,n);
    else
        return _Py_c_quot(c_1, c_powu(x,-n));

}

double
_Py_c_abs(Py_complex z)
{
    /* sets errno = ERANGE on overflow;  otherwise errno = 0 */
    double result;

    if (!isfinite(z.real) || !isfinite(z.imag)) {
        /* C99 rules: if either the real or the imaginary part is an
           infinity, return infinity, even if the other part is a
           NaN. */
        if (isinf(z.real)) {
            result = fabs(z.real);
            errno = 0;
            return result;
        }
        if (isinf(z.imag)) {
            result = fabs(z.imag);
            errno = 0;
            return result;
        }
        /* either the real or imaginary part is a NaN,
           and neither is infinite. Result should be NaN. */
        return Py_NAN;
    }
    result = hypot(z.real, z.imag);
    if (!isfinite(result))
        errno = ERANGE;
    else
        errno = 0;
    return result;
}

static PyObject *
complex_subtype_from_c_complex(PyTypeObject *type, Py_complex cval)
{
    PyObject *op;

    op = type->tp_alloc(type, 0);
    if (op != NULL)
        ((PyComplexObject *)op)->cval = cval;
    return op;
}

PyObject *
PyComplex_FromCComplex(Py_complex cval)
{
    PyComplexObject *op = _Py_FREELIST_POP(PyComplexObject, complexes);

    if (op == NULL) {
        /* Inline PyObject_New */
        op = PyObject_Malloc(sizeof(PyComplexObject));
        if (op == NULL) {
            return PyErr_NoMemory();
        }
        _PyObject_Init((PyObject*)op, &PyComplex_Type);
    }
    op->cval = cval;
    return (PyObject *) op;
}

static void
complex_dealloc(PyObject *op)
{
    assert(PyComplex_Check(op));
    if (PyComplex_CheckExact(op)) {
        _Py_FREELIST_FREE(complexes, op, PyObject_Free);
    }
    else {
        Py_TYPE(op)->tp_free(op);
    }
}

static PyObject *
complex_subtype_from_doubles(PyTypeObject *type, double real, double imag)
{
    Py_complex c;
    c.real = real;
    c.imag = imag;
    return complex_subtype_from_c_complex(type, c);
}

PyObject *
PyComplex_FromDoubles(double real, double imag)
{
    Py_complex c;
    c.real = real;
    c.imag = imag;
    return PyComplex_FromCComplex(c);
}

static PyObject * try_complex_special_method(PyObject *);

double
PyComplex_RealAsDouble(PyObject *op)
{
    double real = -1.0;

    if (PyComplex_Check(op)) {
        real = ((PyComplexObject *)op)->cval.real;
    }
    else {
        PyObject* newop = try_complex_special_method(op);
        if (newop) {
            real = ((PyComplexObject *)newop)->cval.real;
            Py_DECREF(newop);
        } else if (!PyErr_Occurred()) {
            real = PyFloat_AsDouble(op);
        }
    }

    return real;
}

double
PyComplex_ImagAsDouble(PyObject *op)
{
    double imag = -1.0;

    if (PyComplex_Check(op)) {
        imag = ((PyComplexObject *)op)->cval.imag;
    }
    else {
        PyObject* newop = try_complex_special_method(op);
        if (newop) {
            imag = ((PyComplexObject *)newop)->cval.imag;
            Py_DECREF(newop);
        } else if (!PyErr_Occurred()) {
            PyFloat_AsDouble(op);
            if (!PyErr_Occurred()) {
                imag = 0.0;
            }
        }
    }

    return imag;
}

static PyObject *
try_complex_special_method(PyObject *op)
{
    PyObject *f;

    f = _PyObject_LookupSpecial(op, &_Py_ID(__complex__));
    if (f) {
        PyObject *res = _PyObject_CallNoArgs(f);
        Py_DECREF(f);
        if (!res || PyComplex_CheckExact(res)) {
            return res;
        }
        if (!PyComplex_Check(res)) {
            PyErr_Format(PyExc_TypeError,
                "__complex__ returned non-complex (type %.200s)",
                Py_TYPE(res)->tp_name);
            Py_DECREF(res);
            return NULL;
        }
        /* Issue #29894: warn if 'res' not of exact type complex. */
        if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                "__complex__ returned non-complex (type %.200s).  "
                "The ability to return an instance of a strict subclass of complex "
                "is deprecated, and may be removed in a future version of Python.",
                Py_TYPE(res)->tp_name)) {
            Py_DECREF(res);
            return NULL;
        }
        return res;
    }
    return NULL;
}

Py_complex
PyComplex_AsCComplex(PyObject *op)
{
    Py_complex cv;
    PyObject *newop = NULL;

    assert(op);
    /* If op is already of type PyComplex_Type, return its value */
    if (PyComplex_Check(op)) {
        return ((PyComplexObject *)op)->cval;
    }
    /* If not, use op's __complex__  method, if it exists */

    /* return -1 on failure */
    cv.real = -1.;
    cv.imag = 0.;

    newop = try_complex_special_method(op);

    if (newop) {
        cv = ((PyComplexObject *)newop)->cval;
        Py_DECREF(newop);
        return cv;
    }
    else if (PyErr_Occurred()) {
        return cv;
    }
    /* If neither of the above works, interpret op as a float giving the
       real part of the result, and fill in the imaginary part as 0. */
    else {
        /* PyFloat_AsDouble will return -1 on failure */
        cv.real = PyFloat_AsDouble(op);
        return cv;
    }
}

static PyObject *
complex_repr(PyObject *op)
{
    int precision = 0;
    char format_code = 'r';
    PyObject *result = NULL;
    PyComplexObject *v = _PyComplexObject_CAST(op);

    /* If these are non-NULL, they'll need to be freed. */
    char *pre = NULL;
    char *im = NULL;

    /* These do not need to be freed. re is either an alias
       for pre or a pointer to a constant.  lead and tail
       are pointers to constants. */
    const char *re = NULL;
    const char *lead = "";
    const char *tail = "";

    if (v->cval.real == 0. && copysign(1.0, v->cval.real)==1.0) {
        /* Real part is +0: just output the imaginary part and do not
           include parens. */
        re = "";
        im = PyOS_double_to_string(v->cval.imag, format_code,
                                   precision, 0, NULL);
        if (!im) {
            PyErr_NoMemory();
            goto done;
        }
    } else {
        /* Format imaginary part with sign, real part without. Include
           parens in the result. */
        pre = PyOS_double_to_string(v->cval.real, format_code,
                                    precision, 0, NULL);
        if (!pre) {
            PyErr_NoMemory();
            goto done;
        }
        re = pre;

        im = PyOS_double_to_string(v->cval.imag, format_code,
                                   precision, Py_DTSF_SIGN, NULL);
        if (!im) {
            PyErr_NoMemory();
            goto done;
        }
        lead = "(";
        tail = ")";
    }
    result = PyUnicode_FromFormat("%s%s%sj%s", lead, re, im, tail);
  done:
    PyMem_Free(im);
    PyMem_Free(pre);

    return result;
}

static Py_hash_t
complex_hash(PyObject *op)
{
    Py_uhash_t hashreal, hashimag, combined;
    PyComplexObject *v = _PyComplexObject_CAST(op);
    hashreal = (Py_uhash_t)_Py_HashDouble(op, v->cval.real);
    if (hashreal == (Py_uhash_t)-1)
        return -1;
    hashimag = (Py_uhash_t)_Py_HashDouble(op, v->cval.imag);
    if (hashimag == (Py_uhash_t)-1)
        return -1;
    /* Note:  if the imaginary part is 0, hashimag is 0 now,
     * so the following returns hashreal unchanged.  This is
     * important because numbers of different types that
     * compare equal must have the same hash value, so that
     * hash(x + 0*j) must equal hash(x).
     */
    combined = hashreal + _PyHASH_IMAG * hashimag;
    if (combined == (Py_uhash_t)-1)
        combined = (Py_uhash_t)-2;
    return (Py_hash_t)combined;
}

/* This macro may return! */
#define TO_COMPLEX(obj, c)                      \
    if (PyComplex_Check(obj))                   \
        c = ((PyComplexObject *)(obj))->cval;   \
    else if (real_to_complex(&(obj), &(c)) < 0) \
        return (obj)

static int
real_to_double(PyObject **pobj, double *dbl)
{
    PyObject *obj = *pobj;

    if (PyFloat_Check(obj)) {
        *dbl = PyFloat_AS_DOUBLE(obj);
    }
    else if (_Py_convert_int_to_double(pobj, dbl) < 0) {
        return -1;
    }
    return 0;
}

static int
real_to_complex(PyObject **pobj, Py_complex *pc)
{
    pc->imag = 0.0;
    return real_to_double(pobj, &(pc->real));
}

/* Complex arithmetic rules implement special mixed-mode case where combining
   a pure-real (float or int) value and a complex value is performed directly
   without first coercing the real value to a complex value.

   Let us consider the addition as an example, assuming that ints are implicitly
   converted to floats.  We have the following rules (up to variants with changed
   order of operands):

       complex(a, b) + complex(c, d) = complex(a + c, b + d)
       float(a) + complex(b, c) = complex(a + b, c)

   Similar rules are implemented for subtraction, multiplication and division.
   See C11's Annex G, sections G.5.1 and G.5.2.
 */

#define COMPLEX_BINOP(NAME, FUNC)                           \
    static PyObject *                                       \
    complex_##NAME(PyObject *v, PyObject *w)                \
    {                                                       \
        Py_complex a;                                       \
        errno = 0;                                          \
        if (PyComplex_Check(w)) {                           \
            Py_complex b = ((PyComplexObject *)w)->cval;    \
            if (PyComplex_Check(v)) {                       \
                a = ((PyComplexObject *)v)->cval;           \
                a = _Py_c_##FUNC(a, b);                     \
            }                                               \
            else if (real_to_double(&v, &a.real) < 0) {     \
                return v;                                   \
            }                                               \
            else {                                          \
                a = _Py_rc_##FUNC(a.real, b);               \
            }                                               \
        }                                                   \
        else if (!PyComplex_Check(v)) {                     \
            Py_RETURN_NOTIMPLEMENTED;                       \
        }                                                   \
        else {                                              \
            a = ((PyComplexObject *)v)->cval;               \
            double b;                                       \
            if (real_to_double(&w, &b) < 0) {               \
                return w;                                   \
            }                                               \
            a = _Py_cr_##FUNC(a, b);                        \
        }                                                   \
        if (errno == EDOM) {                                \
            PyErr_SetString(PyExc_ZeroDivisionError,        \
                            "division by zero");            \
            return NULL;                                    \
        }                                                   \
        return PyComplex_FromCComplex(a);                   \
   }

COMPLEX_BINOP(add, sum)
COMPLEX_BINOP(mul, prod)
COMPLEX_BINOP(sub, diff)
COMPLEX_BINOP(div, quot)

static PyObject *
complex_pow(PyObject *v, PyObject *w, PyObject *z)
{
    Py_complex p;
    Py_complex a, b;
    TO_COMPLEX(v, a);
    TO_COMPLEX(w, b);

    if (z != Py_None) {
        PyErr_SetString(PyExc_ValueError, "complex modulo");
        return NULL;
    }
    errno = 0;
    // Check whether the exponent has a small integer value, and if so use
    // a faster and more accurate algorithm.
    if (b.imag == 0.0 && b.real == floor(b.real) && fabs(b.real) <= 100.0) {
        p = c_powi(a, (long)b.real);
        _Py_ADJUST_ERANGE2(p.real, p.imag);
    }
    else {
        p = _Py_c_pow(a, b);
    }

    if (errno == EDOM) {
        PyErr_SetString(PyExc_ZeroDivisionError,
                        "zero to a negative or complex power");
        return NULL;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError,
                        "complex exponentiation");
        return NULL;
    }
    return PyComplex_FromCComplex(p);
}

static PyObject *
complex_neg(PyObject *op)
{
    PyComplexObject *v = _PyComplexObject_CAST(op);
    Py_complex neg;
    neg.real = -v->cval.real;
    neg.imag = -v->cval.imag;
    return PyComplex_FromCComplex(neg);
}

static PyObject *
complex_pos(PyObject *op)
{
    PyComplexObject *v = _PyComplexObject_CAST(op);
    if (PyComplex_CheckExact(v)) {
        return Py_NewRef(v);
    }
    return PyComplex_FromCComplex(v->cval);
}

static PyObject *
complex_abs(PyObject *op)
{
    PyComplexObject *v = _PyComplexObject_CAST(op);
    double result = _Py_c_abs(v->cval);
    if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError,
                        "absolute value too large");
        return NULL;
    }
    return PyFloat_FromDouble(result);
}

static int
complex_bool(PyObject *op)
{
    PyComplexObject *v = _PyComplexObject_CAST(op);
    return v->cval.real != 0.0 || v->cval.imag != 0.0;
}

static PyObject *
complex_richcompare(PyObject *v, PyObject *w, int op)
{
    PyObject *res;
    Py_complex i;
    int equal;

    if (op != Py_EQ && op != Py_NE) {
        goto Unimplemented;
    }

    assert(PyComplex_Check(v));
    TO_COMPLEX(v, i);

    if (PyLong_Check(w)) {
        /* Check for 0.0 imaginary part first to avoid the rich
         * comparison when possible.
         */
        if (i.imag == 0.0) {
            PyObject *j, *sub_res;
            j = PyFloat_FromDouble(i.real);
            if (j == NULL)
                return NULL;

            sub_res = PyObject_RichCompare(j, w, op);
            Py_DECREF(j);
            return sub_res;
        }
        else {
            equal = 0;
        }
    }
    else if (PyFloat_Check(w)) {
        equal = (i.real == PyFloat_AsDouble(w) && i.imag == 0.0);
    }
    else if (PyComplex_Check(w)) {
        Py_complex j;

        TO_COMPLEX(w, j);
        equal = (i.real == j.real && i.imag == j.imag);
    }
    else {
        goto Unimplemented;
    }

    if (equal == (op == Py_EQ))
         res = Py_True;
    else
         res = Py_False;

    return Py_NewRef(res);

Unimplemented:
    Py_RETURN_NOTIMPLEMENTED;
}

/*[clinic input]
complex.conjugate

Return the complex conjugate of its argument. (3-4j).conjugate() == 3+4j.
[clinic start generated code]*/

static PyObject *
complex_conjugate_impl(PyComplexObject *self)
/*[clinic end generated code: output=5059ef162edfc68e input=5fea33e9747ec2c4]*/
{
    Py_complex c = self->cval;
    c.imag = -c.imag;
    return PyComplex_FromCComplex(c);
}

/*[clinic input]
complex.__getnewargs__

[clinic start generated code]*/

static PyObject *
complex___getnewargs___impl(PyComplexObject *self)
/*[clinic end generated code: output=689b8206e8728934 input=539543e0a50533d7]*/
{
    Py_complex c = self->cval;
    return Py_BuildValue("(dd)", c.real, c.imag);
}


/*[clinic input]
complex.__format__

    format_spec: unicode
    /

Convert to a string according to format_spec.
[clinic start generated code]*/

static PyObject *
complex___format___impl(PyComplexObject *self, PyObject *format_spec)
/*[clinic end generated code: output=bfcb60df24cafea0 input=014ef5488acbe1d5]*/
{
    _PyUnicodeWriter writer;
    int ret;
    _PyUnicodeWriter_Init(&writer);
    ret = _PyComplex_FormatAdvancedWriter(
        &writer,
        (PyObject *)self,
        format_spec, 0, PyUnicode_GET_LENGTH(format_spec));
    if (ret == -1) {
        _PyUnicodeWriter_Dealloc(&writer);
        return NULL;
    }
    return _PyUnicodeWriter_Finish(&writer);
}

/*[clinic input]
complex.__complex__

Convert this value to exact type complex.
[clinic start generated code]*/

static PyObject *
complex___complex___impl(PyComplexObject *self)
/*[clinic end generated code: output=e6b35ba3d275dc9c input=3589ada9d27db854]*/
{
    if (PyComplex_CheckExact(self)) {
        return Py_NewRef(self);
    }
    else {
        return PyComplex_FromCComplex(self->cval);
    }
}


static PyObject *
complex_from_string_inner(const char *s, Py_ssize_t len, void *type)
{
    double x=0.0, y=0.0, z;
    int got_bracket=0;
    const char *start;
    char *end;

    /* position on first nonblank */
    start = s;
    while (Py_ISSPACE(*s))
        s++;
    if (*s == '(') {
        /* Skip over possible bracket from repr(). */
        got_bracket = 1;
        s++;
        while (Py_ISSPACE(*s))
            s++;
    }

    /* a valid complex string usually takes one of the three forms:

         <float>                  - real part only
         <float>j                 - imaginary part only
         <float><signed-float>j   - real and imaginary parts

       where <float> represents any numeric string that's accepted by the
       float constructor (including 'nan', 'inf', 'infinity', etc.), and
       <signed-float> is any string of the form <float> whose first
       character is '+' or '-'.

       For backwards compatibility, the extra forms

         <float><sign>j
         <sign>j
         j

       are also accepted, though support for these forms may be removed from
       a future version of Python.
    */

    /* first look for forms starting with <float> */
    z = PyOS_string_to_double(s, &end, NULL);
    if (z == -1.0 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_ValueError))
            PyErr_Clear();
        else
            return NULL;
    }
    if (end != s) {
        /* all 4 forms starting with <float> land here */
        s = end;
        if (*s == '+' || *s == '-') {
            /* <float><signed-float>j | <float><sign>j */
            x = z;
            y = PyOS_string_to_double(s, &end, NULL);
            if (y == -1.0 && PyErr_Occurred()) {
                if (PyErr_ExceptionMatches(PyExc_ValueError))
                    PyErr_Clear();
                else
                    return NULL;
            }
            if (end != s)
                /* <float><signed-float>j */
                s = end;
            else {
                /* <float><sign>j */
                y = *s == '+' ? 1.0 : -1.0;
                s++;
            }
            if (!(*s == 'j' || *s == 'J'))
                goto parse_error;
            s++;
        }
        else if (*s == 'j' || *s == 'J') {
            /* <float>j */
            s++;
            y = z;
        }
        else
            /* <float> */
            x = z;
    }
    else {
        /* not starting with <float>; must be <sign>j or j */
        if (*s == '+' || *s == '-') {
            /* <sign>j */
            y = *s == '+' ? 1.0 : -1.0;
            s++;
        }
        else
            /* j */
            y = 1.0;
        if (!(*s == 'j' || *s == 'J'))
            goto parse_error;
        s++;
    }

    /* trailing whitespace and closing bracket */
    while (Py_ISSPACE(*s))
        s++;
    if (got_bracket) {
        /* if there was an opening parenthesis, then the corresponding
           closing parenthesis should be right here */
        if (*s != ')')
            goto parse_error;
        s++;
        while (Py_ISSPACE(*s))
            s++;
    }

    /* we should now be at the end of the string */
    if (s-start != len)
        goto parse_error;

    return complex_subtype_from_doubles(_PyType_CAST(type), x, y);

  parse_error:
    PyErr_SetString(PyExc_ValueError,
                    "complex() arg is a malformed string");
    return NULL;
}

static PyObject *
complex_subtype_from_string(PyTypeObject *type, PyObject *v)
{
    const char *s;
    PyObject *s_buffer = NULL, *result = NULL;
    Py_ssize_t len;

    if (PyUnicode_Check(v)) {
        s_buffer = _PyUnicode_TransformDecimalAndSpaceToASCII(v);
        if (s_buffer == NULL) {
            return NULL;
        }
        assert(PyUnicode_IS_ASCII(s_buffer));
        /* Simply get a pointer to existing ASCII characters. */
        s = PyUnicode_AsUTF8AndSize(s_buffer, &len);
        assert(s != NULL);
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "complex() argument must be a string or a number, not %T",
            v);
        return NULL;
    }

    result = _Py_string_to_number_with_underscores(s, len, "complex", v, type,
                                                   complex_from_string_inner);
    Py_DECREF(s_buffer);
    return result;
}

/* The constructor should only accept a string as a positional argument,
 * not as by the 'real' keyword.  But Argument Clinic does not allow
 * to distinguish between argument passed positionally and by keyword.
 * So the constructor must be split into two parts: actual_complex_new()
 * handles the case of no arguments and one positional argument, and calls
 * complex_new(), implemented with Argument Clinic, to handle the remaining
 * cases: 'real' and 'imag' arguments.  This separation is well suited
 * for different constructor roles: converting a string or number to a complex
 * number and constructing a complex number from real and imaginary parts.
 */
static PyObject *
actual_complex_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *res = NULL;
    PyNumberMethods *nbr;

    if (PyTuple_GET_SIZE(args) > 1 || (kwargs != NULL && PyDict_GET_SIZE(kwargs))) {
        return complex_new(type, args, kwargs);
    }
    if (!PyTuple_GET_SIZE(args)) {
        return complex_subtype_from_doubles(type, 0, 0);
    }

    PyObject *arg = PyTuple_GET_ITEM(args, 0);
    /* Special-case for a single argument when type(arg) is complex. */
    if (PyComplex_CheckExact(arg) && type == &PyComplex_Type) {
        /* Note that we can't know whether it's safe to return
           a complex *subclass* instance as-is, hence the restriction
           to exact complexes here.  If either the input or the
           output is a complex subclass, it will be handled below
           as a non-orthogonal vector.  */
        return Py_NewRef(arg);
    }
    if (PyUnicode_Check(arg)) {
        return complex_subtype_from_string(type, arg);
    }
    PyObject *tmp = try_complex_special_method(arg);
    if (tmp) {
        Py_complex c = ((PyComplexObject*)tmp)->cval;
        res = complex_subtype_from_doubles(type, c.real, c.imag);
        Py_DECREF(tmp);
    }
    else if (PyErr_Occurred()) {
        return NULL;
    }
    else if (PyComplex_Check(arg)) {
        /* Note that if arg is of a complex subtype, we're only
           retaining its real & imag parts here, and the return
           value is (properly) of the builtin complex type. */
        Py_complex c = ((PyComplexObject*)arg)->cval;
        res = complex_subtype_from_doubles(type, c.real, c.imag);
    }
    else if ((nbr = Py_TYPE(arg)->tp_as_number) != NULL &&
             (nbr->nb_float != NULL || nbr->nb_index != NULL))
    {
        /* The argument really is entirely real, and contributes
           nothing in the imaginary direction.
           Just treat it as a double. */
        double r = PyFloat_AsDouble(arg);
        if (r != -1.0 || !PyErr_Occurred()) {
            res = complex_subtype_from_doubles(type, r, 0);
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "complex() argument must be a string or a number, not %T",
                     arg);
    }
    return res;
}

/*[clinic input]
@classmethod
complex.__new__ as complex_new
    real as r: object(c_default="NULL") = 0
    imag as i: object(c_default="NULL") = 0

Create a complex number from a string or numbers.

If a string is given, parse it as a complex number.
If a single number is given, convert it to a complex number.
If the 'real' or 'imag' arguments are given, create a complex number
with the specified real and imaginary components.
[clinic start generated code]*/

static PyObject *
complex_new_impl(PyTypeObject *type, PyObject *r, PyObject *i)
/*[clinic end generated code: output=b6c7dd577b537dc1 input=ff4268dc540958a4]*/
{
    PyObject *tmp;
    PyNumberMethods *nbr, *nbi = NULL;
    Py_complex cr, ci;
    int own_r = 0;
    int cr_is_complex = 0;
    int ci_is_complex = 0;

    if (r == NULL) {
        r = _PyLong_GetZero();
    }
    PyObject *orig_r = r;

    /* DEPRECATED: The call of try_complex_special_method() for the "real"
     * part will be dropped after the end of the deprecation period. */
    tmp = try_complex_special_method(r);
    if (tmp) {
        r = tmp;
        own_r = 1;
    }
    else if (PyErr_Occurred()) {
        return NULL;
    }

    nbr = Py_TYPE(r)->tp_as_number;
    if (nbr == NULL ||
        (nbr->nb_float == NULL && nbr->nb_index == NULL && !PyComplex_Check(r)))
    {
        PyErr_Format(PyExc_TypeError,
                     "complex() argument 'real' must be a real number, not %T",
                     r);
        if (own_r) {
            Py_DECREF(r);
        }
        return NULL;
    }
    if (i != NULL) {
        nbi = Py_TYPE(i)->tp_as_number;
        if (nbi == NULL ||
            (nbi->nb_float == NULL && nbi->nb_index == NULL && !PyComplex_Check(i)))
        {
            PyErr_Format(PyExc_TypeError,
                         "complex() argument 'imag' must be a real number, not %T",
                         i);
            if (own_r) {
                Py_DECREF(r);
            }
            return NULL;
        }
    }

    /* If we get this far, then the "real" and "imag" parts should
       both be treated as numbers, and the constructor should return a
       complex number equal to (real + imag*1j).

       The following is DEPRECATED:
       Note that we do NOT assume the input to already be in canonical
       form; the "real" and "imag" parts might themselves be complex
       numbers, which slightly complicates the code below. */
    if (PyComplex_Check(r)) {
        /* Note that if r is of a complex subtype, we're only
           retaining its real & imag parts here, and the return
           value is (properly) of the builtin complex type. */
        cr = ((PyComplexObject*)r)->cval;
        cr_is_complex = 1;
        if (own_r) {
            /* r was a newly created complex number, rather
               than the original "real" argument. */
            Py_DECREF(r);
        }
        nbr = Py_TYPE(orig_r)->tp_as_number;
        if (nbr == NULL ||
            (nbr->nb_float == NULL && nbr->nb_index == NULL))
        {
            if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                    "complex() argument 'real' must be a real number, not %T",
                    orig_r)) {
                return NULL;
            }
        }
    }
    else {
        /* The "real" part really is entirely real, and contributes
           nothing in the imaginary direction.
           Just treat it as a double. */
        tmp = PyNumber_Float(r);
        assert(!own_r);
        if (tmp == NULL)
            return NULL;
        assert(PyFloat_Check(tmp));
        cr.real = PyFloat_AsDouble(tmp);
        cr.imag = 0.0;
        Py_DECREF(tmp);
    }
    if (i == NULL) {
        ci.real = cr.imag;
    }
    else if (PyComplex_Check(i)) {
        if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                "complex() argument 'imag' must be a real number, not %T",
                i)) {
            return NULL;
        }
        ci = ((PyComplexObject*)i)->cval;
        ci_is_complex = 1;
    } else {
        /* The "imag" part really is entirely imaginary, and
           contributes nothing in the real direction.
           Just treat it as a double. */
        tmp = PyNumber_Float(i);
        if (tmp == NULL)
            return NULL;
        ci.real = PyFloat_AsDouble(tmp);
        Py_DECREF(tmp);
    }
    /*  If the input was in canonical form, then the "real" and "imag"
        parts are real numbers, so that ci.imag and cr.imag are zero.
        We need this correction in case they were not real numbers. */

    if (ci_is_complex) {
        cr.real -= ci.imag;
    }
    if (cr_is_complex && i != NULL) {
        ci.real += cr.imag;
    }
    return complex_subtype_from_doubles(type, cr.real, ci.real);
}

/*[clinic input]
@classmethod
complex.from_number

    number: object
    /

Convert number to a complex floating-point number.
[clinic start generated code]*/

static PyObject *
complex_from_number_impl(PyTypeObject *type, PyObject *number)
/*[clinic end generated code: output=7248bb593e1871e1 input=3f8bdd3a2bc3facd]*/
{
    if (PyComplex_CheckExact(number) && type == &PyComplex_Type) {
        Py_INCREF(number);
        return number;
    }
    Py_complex cv = PyComplex_AsCComplex(number);
    if (cv.real == -1.0 && PyErr_Occurred()) {
        return NULL;
    }
    PyObject *result = PyComplex_FromCComplex(cv);
    if (type != &PyComplex_Type && result != NULL) {
        Py_SETREF(result, PyObject_CallOneArg((PyObject *)type, result));
    }
    return result;
}

static PyMethodDef complex_methods[] = {
    COMPLEX_FROM_NUMBER_METHODDEF
    COMPLEX_CONJUGATE_METHODDEF
    COMPLEX___COMPLEX___METHODDEF
    COMPLEX___GETNEWARGS___METHODDEF
    COMPLEX___FORMAT___METHODDEF
    {NULL,              NULL}           /* sentinel */
};

static PyMemberDef complex_members[] = {
    {"real", Py_T_DOUBLE, offsetof(PyComplexObject, cval.real), Py_READONLY,
     "the real part of a complex number"},
    {"imag", Py_T_DOUBLE, offsetof(PyComplexObject, cval.imag), Py_READONLY,
     "the imaginary part of a complex number"},
    {0},
};

static PyNumberMethods complex_as_number = {
    complex_add,                                /* nb_add */
    complex_sub,                                /* nb_subtract */
    complex_mul,                                /* nb_multiply */
    0,                                          /* nb_remainder */
    0,                                          /* nb_divmod */
    complex_pow,                                /* nb_power */
    complex_neg,                                /* nb_negative */
    complex_pos,                                /* nb_positive */
    complex_abs,                                /* nb_absolute */
    complex_bool,                               /* nb_bool */
    0,                                          /* nb_invert */
    0,                                          /* nb_lshift */
    0,                                          /* nb_rshift */
    0,                                          /* nb_and */
    0,                                          /* nb_xor */
    0,                                          /* nb_or */
    0,                                          /* nb_int */
    0,                                          /* nb_reserved */
    0,                                          /* nb_float */
    0,                                          /* nb_inplace_add */
    0,                                          /* nb_inplace_subtract */
    0,                                          /* nb_inplace_multiply*/
    0,                                          /* nb_inplace_remainder */
    0,                                          /* nb_inplace_power */
    0,                                          /* nb_inplace_lshift */
    0,                                          /* nb_inplace_rshift */
    0,                                          /* nb_inplace_and */
    0,                                          /* nb_inplace_xor */
    0,                                          /* nb_inplace_or */
    0,                                          /* nb_floor_divide */
    complex_div,                                /* nb_true_divide */
    0,                                          /* nb_inplace_floor_divide */
    0,                                          /* nb_inplace_true_divide */
};

PyTypeObject PyComplex_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "complex",
    sizeof(PyComplexObject),
    0,
    complex_dealloc,                            /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    complex_repr,                               /* tp_repr */
    &complex_as_number,                         /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    complex_hash,                               /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    complex_new__doc__,                         /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    complex_richcompare,                        /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    complex_methods,                            /* tp_methods */
    complex_members,                            /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    actual_complex_new,                         /* tp_new */
    PyObject_Free,                              /* tp_free */
    .tp_version_tag = _Py_TYPE_VERSION_COMPLEX,
};
