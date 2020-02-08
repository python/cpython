
/* Complex object implementation */

/* Borrows heavily from floatobject.c */

/* Submitted by Jim Hugunin */

#include "Python.h"
#include "structmember.h"

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
_Py_c_diff(Py_complex a, Py_complex b)
{
    Py_complex r;
    r.real = a.real - b.real;
    r.imag = a.imag - b.imag;
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
_Py_c_prod(Py_complex a, Py_complex b)
{
    Py_complex r;
    r.real = a.real*b.real - a.imag*b.imag;
    r.imag = a.real*b.imag + a.imag*b.real;
    return r;
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
     * University).  As usual, though, we're still ignoring all IEEE
     * endcases.
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
            len /= exp(at*b.imag);
            phase += b.imag*log(vabs);
        }
        r.real = len*cos(phase);
        r.imag = len*sin(phase);
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
    Py_complex cn;

    if (n > 100 || n < -100) {
        cn.real = (double) n;
        cn.imag = 0.;
        return _Py_c_pow(x,cn);
    }
    else if (n > 0)
        return c_powu(x,n);
    else
        return _Py_c_quot(c_1, c_powu(x,-n));

}

double
_Py_c_abs(Py_complex z)
{
    /* sets errno = ERANGE on overflow;  otherwise errno = 0 */
    double result;

    if (!Py_IS_FINITE(z.real) || !Py_IS_FINITE(z.imag)) {
        /* C99 rules: if either the real or the imaginary part is an
           infinity, return infinity, even if the other part is a
           NaN. */
        if (Py_IS_INFINITY(z.real)) {
            result = fabs(z.real);
            errno = 0;
            return result;
        }
        if (Py_IS_INFINITY(z.imag)) {
            result = fabs(z.imag);
            errno = 0;
            return result;
        }
        /* either the real or imaginary part is a NaN,
           and neither is infinite. Result should be NaN. */
        return Py_NAN;
    }
    result = hypot(z.real, z.imag);
    if (!Py_IS_FINITE(result))
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
    PyComplexObject *op;

    /* Inline PyObject_New */
    op = (PyComplexObject *) PyObject_MALLOC(sizeof(PyComplexObject));
    if (op == NULL)
        return PyErr_NoMemory();
    (void)PyObject_INIT(op, &PyComplex_Type);
    op->cval = cval;
    return (PyObject *) op;
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

double
PyComplex_RealAsDouble(PyObject *op)
{
    if (PyComplex_Check(op)) {
        return ((PyComplexObject *)op)->cval.real;
    }
    else {
        return PyFloat_AsDouble(op);
    }
}

double
PyComplex_ImagAsDouble(PyObject *op)
{
    if (PyComplex_Check(op)) {
        return ((PyComplexObject *)op)->cval.imag;
    }
    else {
        return 0.0;
    }
}

static PyObject *
try_complex_special_method(PyObject *op)
{
    PyObject *f;
    _Py_IDENTIFIER(__complex__);

    f = _PyObject_LookupSpecial(op, &PyId___complex__);
    if (f) {
        PyObject *res = _PyObject_CallNoArg(f);
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
complex_repr(PyComplexObject *v)
{
    int precision = 0;
    char format_code = 'r';
    PyObject *result = NULL;

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
complex_hash(PyComplexObject *v)
{
    Py_uhash_t hashreal, hashimag, combined;
    hashreal = (Py_uhash_t)_Py_HashDouble(v->cval.real);
    if (hashreal == (Py_uhash_t)-1)
        return -1;
    hashimag = (Py_uhash_t)_Py_HashDouble(v->cval.imag);
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
#define TO_COMPLEX(obj, c) \
    if (PyComplex_Check(obj)) \
        c = ((PyComplexObject *)(obj))->cval; \
    else if (to_complex(&(obj), &(c)) < 0) \
        return (obj)

static int
to_complex(PyObject **pobj, Py_complex *pc)
{
    PyObject *obj = *pobj;

    pc->real = pc->imag = 0.0;
    if (PyLong_Check(obj)) {
        pc->real = PyLong_AsDouble(obj);
        if (pc->real == -1.0 && PyErr_Occurred()) {
            *pobj = NULL;
            return -1;
        }
        return 0;
    }
    if (PyFloat_Check(obj)) {
        pc->real = PyFloat_AsDouble(obj);
        return 0;
    }
    Py_INCREF(Py_NotImplemented);
    *pobj = Py_NotImplemented;
    return -1;
}


static PyObject *
complex_add(PyObject *v, PyObject *w)
{
    Py_complex result;
    Py_complex a, b;
    TO_COMPLEX(v, a);
    TO_COMPLEX(w, b);
    result = _Py_c_sum(a, b);
    return PyComplex_FromCComplex(result);
}

static PyObject *
complex_sub(PyObject *v, PyObject *w)
{
    Py_complex result;
    Py_complex a, b;
    TO_COMPLEX(v, a);
    TO_COMPLEX(w, b);
    result = _Py_c_diff(a, b);
    return PyComplex_FromCComplex(result);
}

static PyObject *
complex_mul(PyObject *v, PyObject *w)
{
    Py_complex result;
    Py_complex a, b;
    TO_COMPLEX(v, a);
    TO_COMPLEX(w, b);
    result = _Py_c_prod(a, b);
    return PyComplex_FromCComplex(result);
}

static PyObject *
complex_div(PyObject *v, PyObject *w)
{
    Py_complex quot;
    Py_complex a, b;
    TO_COMPLEX(v, a);
    TO_COMPLEX(w, b);
    errno = 0;
    quot = _Py_c_quot(a, b);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ZeroDivisionError, "complex division by zero");
        return NULL;
    }
    return PyComplex_FromCComplex(quot);
}

static PyObject *
complex_remainder(PyObject *v, PyObject *w)
{
    PyErr_SetString(PyExc_TypeError,
                    "can't mod complex numbers.");
    return NULL;
}


static PyObject *
complex_divmod(PyObject *v, PyObject *w)
{
    PyErr_SetString(PyExc_TypeError,
                    "can't take floor or mod of complex number.");
    return NULL;
}

static PyObject *
complex_pow(PyObject *v, PyObject *w, PyObject *z)
{
    Py_complex p;
    Py_complex exponent;
    long int_exponent;
    Py_complex a, b;
    TO_COMPLEX(v, a);
    TO_COMPLEX(w, b);

    if (z != Py_None) {
        PyErr_SetString(PyExc_ValueError, "complex modulo");
        return NULL;
    }
    errno = 0;
    exponent = b;
    int_exponent = (long)exponent.real;
    if (exponent.imag == 0. && exponent.real == int_exponent)
        p = c_powi(a, int_exponent);
    else
        p = _Py_c_pow(a, exponent);

    Py_ADJUST_ERANGE2(p.real, p.imag);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ZeroDivisionError,
                        "0.0 to a negative or complex power");
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
complex_int_div(PyObject *v, PyObject *w)
{
    PyErr_SetString(PyExc_TypeError,
                    "can't take floor of complex number.");
    return NULL;
}

static PyObject *
complex_neg(PyComplexObject *v)
{
    Py_complex neg;
    neg.real = -v->cval.real;
    neg.imag = -v->cval.imag;
    return PyComplex_FromCComplex(neg);
}

static PyObject *
complex_pos(PyComplexObject *v)
{
    if (PyComplex_CheckExact(v)) {
        Py_INCREF(v);
        return (PyObject *)v;
    }
    else
        return PyComplex_FromCComplex(v->cval);
}

static PyObject *
complex_abs(PyComplexObject *v)
{
    double result;

    result = _Py_c_abs(v->cval);

    if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError,
                        "absolute value too large");
        return NULL;
    }
    return PyFloat_FromDouble(result);
}

static int
complex_bool(PyComplexObject *v)
{
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

    Py_INCREF(res);
    return res;

Unimplemented:
    Py_RETURN_NOTIMPLEMENTED;
}

static PyObject *
complex_int(PyObject *v)
{
    PyErr_SetString(PyExc_TypeError,
               "can't convert complex to int");
    return NULL;
}

static PyObject *
complex_float(PyObject *v)
{
    PyErr_SetString(PyExc_TypeError,
               "can't convert complex to float");
    return NULL;
}

static PyObject *
complex_conjugate(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    Py_complex c;
    c = ((PyComplexObject *)self)->cval;
    c.imag = -c.imag;
    return PyComplex_FromCComplex(c);
}

PyDoc_STRVAR(complex_conjugate_doc,
"complex.conjugate() -> complex\n"
"\n"
"Return the complex conjugate of its argument. (3-4j).conjugate() == 3+4j.");

static PyObject *
complex_getnewargs(PyComplexObject *v, PyObject *Py_UNUSED(ignored))
{
    Py_complex c = v->cval;
    return Py_BuildValue("(dd)", c.real, c.imag);
}

PyDoc_STRVAR(complex__format__doc,
"complex.__format__() -> str\n"
"\n"
"Convert to a string according to format_spec.");

static PyObject *
complex__format__(PyObject* self, PyObject* args)
{
    PyObject *format_spec;
    _PyUnicodeWriter writer;
    int ret;

    if (!PyArg_ParseTuple(args, "U:__format__", &format_spec))
        return NULL;

    _PyUnicodeWriter_Init(&writer);
    ret = _PyComplex_FormatAdvancedWriter(
        &writer,
        self,
        format_spec, 0, PyUnicode_GET_LENGTH(format_spec));
    if (ret == -1) {
        _PyUnicodeWriter_Dealloc(&writer);
        return NULL;
    }
    return _PyUnicodeWriter_Finish(&writer);
}

static PyMethodDef complex_methods[] = {
    {"conjugate",       (PyCFunction)complex_conjugate, METH_NOARGS,
     complex_conjugate_doc},
    {"__getnewargs__",          (PyCFunction)complex_getnewargs,        METH_NOARGS},
    {"__format__",          (PyCFunction)complex__format__,
                                       METH_VARARGS, complex__format__doc},
    {NULL,              NULL}           /* sentinel */
};

static PyMemberDef complex_members[] = {
    {"real", T_DOUBLE, offsetof(PyComplexObject, cval.real), READONLY,
     "the real part of a complex number"},
    {"imag", T_DOUBLE, offsetof(PyComplexObject, cval.imag), READONLY,
     "the imaginary part of a complex number"},
    {0},
};

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

    return complex_subtype_from_doubles((PyTypeObject *)type, x, y);

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
            "complex() argument must be a string or a number, not '%.200s'",
            Py_TYPE(v)->tp_name);
        return NULL;
    }

    result = _Py_string_to_number_with_underscores(s, len, "complex", v, type,
                                                   complex_from_string_inner);
    Py_DECREF(s_buffer);
    return result;
}

/*[clinic input]
@classmethod
complex.__new__ as complex_new
    real as r: object(c_default="_PyLong_Zero") = 0
    imag as i: object(c_default="NULL") = 0

Create a complex number from a real part and an optional imaginary part.

This is equivalent to (real + imag*1j) where imag defaults to 0.
[clinic start generated code]*/

static PyObject *
complex_new_impl(PyTypeObject *type, PyObject *r, PyObject *i)
/*[clinic end generated code: output=b6c7dd577b537dc1 input=6f6b0bedba29bcb5]*/
{
    PyObject *tmp;
    PyNumberMethods *nbr, *nbi = NULL;
    Py_complex cr, ci;
    int own_r = 0;
    int cr_is_complex = 0;
    int ci_is_complex = 0;

    /* Special-case for a single argument when type(arg) is complex. */
    if (PyComplex_CheckExact(r) && i == NULL &&
        type == &PyComplex_Type) {
        /* Note that we can't know whether it's safe to return
           a complex *subclass* instance as-is, hence the restriction
           to exact complexes here.  If either the input or the
           output is a complex subclass, it will be handled below
           as a non-orthogonal vector.  */
        Py_INCREF(r);
        return r;
    }
    if (PyUnicode_Check(r)) {
        if (i != NULL) {
            PyErr_SetString(PyExc_TypeError,
                            "complex() can't take second arg"
                            " if first is a string");
            return NULL;
        }
        return complex_subtype_from_string(type, r);
    }
    if (i != NULL && PyUnicode_Check(i)) {
        PyErr_SetString(PyExc_TypeError,
                        "complex() second arg can't be a string");
        return NULL;
    }

    tmp = try_complex_special_method(r);
    if (tmp) {
        r = tmp;
        own_r = 1;
    }
    else if (PyErr_Occurred()) {
        return NULL;
    }

    nbr = Py_TYPE(r)->tp_as_number;
    if (nbr == NULL || (nbr->nb_float == NULL && nbr->nb_index == NULL)) {
        PyErr_Format(PyExc_TypeError,
                     "complex() first argument must be a string or a number, "
                     "not '%.200s'",
                     Py_TYPE(r)->tp_name);
        if (own_r) {
            Py_DECREF(r);
        }
        return NULL;
    }
    if (i != NULL) {
        nbi = Py_TYPE(i)->tp_as_number;
        if (nbi == NULL || (nbi->nb_float == NULL && nbi->nb_index == NULL)) {
            PyErr_Format(PyExc_TypeError,
                         "complex() second argument must be a number, "
                         "not '%.200s'",
                         Py_TYPE(i)->tp_name);
            if (own_r) {
                Py_DECREF(r);
            }
            return NULL;
        }
    }

    /* If we get this far, then the "real" and "imag" parts should
       both be treated as numbers, and the constructor should return a
       complex number equal to (real + imag*1j).

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
            Py_DECREF(r);
        }
    }
    else {
        /* The "real" part really is entirely real, and contributes
           nothing in the imaginary direction.
           Just treat it as a double. */
        tmp = PyNumber_Float(r);
        if (own_r) {
            /* r was a newly created complex number, rather
               than the original "real" argument. */
            Py_DECREF(r);
        }
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

static PyNumberMethods complex_as_number = {
    (binaryfunc)complex_add,                    /* nb_add */
    (binaryfunc)complex_sub,                    /* nb_subtract */
    (binaryfunc)complex_mul,                    /* nb_multiply */
    (binaryfunc)complex_remainder,              /* nb_remainder */
    (binaryfunc)complex_divmod,                 /* nb_divmod */
    (ternaryfunc)complex_pow,                   /* nb_power */
    (unaryfunc)complex_neg,                     /* nb_negative */
    (unaryfunc)complex_pos,                     /* nb_positive */
    (unaryfunc)complex_abs,                     /* nb_absolute */
    (inquiry)complex_bool,                      /* nb_bool */
    0,                                          /* nb_invert */
    0,                                          /* nb_lshift */
    0,                                          /* nb_rshift */
    0,                                          /* nb_and */
    0,                                          /* nb_xor */
    0,                                          /* nb_or */
    complex_int,                                /* nb_int */
    0,                                          /* nb_reserved */
    complex_float,                              /* nb_float */
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
    (binaryfunc)complex_int_div,                /* nb_floor_divide */
    (binaryfunc)complex_div,                    /* nb_true_divide */
    0,                                          /* nb_inplace_floor_divide */
    0,                                          /* nb_inplace_true_divide */
};

PyTypeObject PyComplex_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "complex",
    sizeof(PyComplexObject),
    0,
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)complex_repr,                     /* tp_repr */
    &complex_as_number,                         /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    (hashfunc)complex_hash,                     /* tp_hash */
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
    complex_new,                                /* tp_new */
    PyObject_Del,                               /* tp_free */
};
