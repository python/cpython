#include "Python.h"
#ifdef MS_WINDOWS
#include <windows.h>
#endif

#if defined(__APPLE__) && defined(HAVE_GETTIMEOFDAY) && defined(HAVE_FTIME)
  /*
   * _PyTime_gettimeofday falls back to ftime when getttimeofday fails because the latter
   * might fail on some platforms. This fallback is unwanted on MacOSX because
   * that makes it impossible to use a binary build on OSX 10.4 on earlier
   * releases of the OS. Therefore claim we don't support ftime.
   */
# undef HAVE_FTIME
#endif

#if defined(HAVE_FTIME) && !defined(MS_WINDOWS)
#include <sys/timeb.h>
extern int ftime(struct timeb *);
#endif

#define MICROSECONDS    1000000

void
_PyTime_get(_PyTime_t *ts)
{
#ifdef MS_WINDOWS
    FILETIME system_time;
    ULARGE_INTEGER large;
    ULONGLONG value;

    GetSystemTimeAsFileTime(&system_time);
    large.u.LowPart = system_time.dwLowDateTime;
    large.u.HighPart = system_time.dwHighDateTime;
    /* 116,444,736,000,000,000: number of 100 ns between
       the 1st january 1601 and the 1st january 1970 (369 years + 89 leap
       days). */
    value = large.QuadPart - 116444736000000000;
    ts->seconds = 0;
    ts->numerator = value;
    ts->denominator = (_PyTime_fraction_t)10000000;
#else

#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
    int err;
#endif
#if defined(HAVE_FTIME)
    struct timeb t;
#endif

    /* There are three ways to get the time:
      (1) gettimeofday() -- resolution in microseconds
      (2) ftime() -- resolution in milliseconds
      (3) time() -- resolution in seconds
      In all cases the return value in a timeval struct.
      Since on some systems (e.g. SCO ODT 3.0) gettimeofday() may
      fail, so we fall back on ftime() or time().
      Note: clock resolution does not imply clock accuracy! */

#ifdef HAVE_GETTIMEOFDAY
#ifdef GETTIMEOFDAY_NO_TZ
    err = gettimeofday(&tv);
#else /* !GETTIMEOFDAY_NO_TZ */
    err = gettimeofday(&tv, (struct timezone *)NULL);
#endif /* !GETTIMEOFDAY_NO_TZ */
    if (err == 0)
    {
        ts->seconds = tv.tv_sec;
        ts->numerator = tv.tv_usec;
        ts->denominator = MICROSECONDS;
        return;
    }
#endif /* !HAVE_GETTIMEOFDAY */

#if defined(HAVE_FTIME)
    ftime(&t);
    ts->seconds = t.time;
    ts->numerator = t.millitm;
    ts->denominator = 1000;
#else /* !HAVE_FTIME */
    ts->seconds = time(NULL);
    ts->numerator = 0;
    ts->denominator = 1;
#endif /* !HAVE_FTIME */

#endif /* MS_WINDOWS */
}

void
_PyTime_gettimeofday(_PyTime_timeval *tv)
{
    _PyTime_t ts;
    _PyTime_fraction_t k;
    time_t sec;

    _PyTime_get(&ts);
    tv->tv_sec = ts.seconds;
    if (ts.numerator) {
        if (ts.numerator > ts.denominator) {
            sec = Py_SAFE_DOWNCAST(ts.numerator / ts.denominator,
                                   _PyTime_fraction_t, time_t);
            /* ignore integer overflow because _PyTime_gettimeofday() has
               no return value */
            tv->tv_sec += sec;
            ts.numerator = ts.numerator % ts.denominator;
        }
        if (MICROSECONDS >= ts.denominator) {
            k = (_PyTime_fraction_t)MICROSECONDS / ts.denominator;
            tv->tv_usec = (long)(ts.numerator * k);
        }
        else {
            k = ts.denominator / (_PyTime_fraction_t)MICROSECONDS;
            tv->tv_usec = (long)(ts.numerator / k);
        }
    }
    else {
        tv->tv_usec = 0;
    }
}

static PyObject*
_PyLong_FromTime_t(time_t value)
{
#if SIZEOF_TIME_T <= SIZEOF_LONG
    return PyLong_FromLong(value);
#else
    assert(sizeof(time_t) <= sizeof(PY_LONG_LONG));
    return PyLong_FromLongLong(value);
#endif
}

#if defined(HAVE_LONG_LONG)
#  define _PyLong_FromTimeFraction_t PyLong_FromLongLong
#else
#  define _PyLong_FromTimeFraction_t PyLong_FromSize_t
#endif

/* Convert a timestamp to a PyFloat object */
static PyObject*
_PyTime_AsFloat(_PyTime_t *ts)
{
    double d;
    d = (double)ts->seconds;
    d += (double)ts->numerator / (double)ts->denominator;
    return PyFloat_FromDouble(d);
}

/* Convert a timestamp to a PyLong object */
static PyObject*
_PyTime_AsLong(_PyTime_t *ts)
{
    PyObject *a, *b, *c;

    a = _PyLong_FromTime_t(ts->seconds);
    if (a == NULL)
        return NULL;
    b = _PyLong_FromTimeFraction_t(ts->numerator / ts->denominator);
    if (b == NULL)
    {
        Py_DECREF(a);
        return NULL;
    }
    c = PyNumber_Add(a, b);
    Py_DECREF(a);
    Py_DECREF(b);
    return c;
}

/* Convert a timestamp to a decimal.Decimal object */
static PyObject*
_PyTime_AsDecimal(_PyTime_t *ts)
{
    static PyObject* module = NULL;
    static PyObject* decimal = NULL;
    static PyObject* exponent_context = NULL;
    static PyObject* context = NULL;
    /* exponent cache, dictionary of:
       int (denominator) => Decimal (1/denominator) */
    static PyObject* exponent_cache = NULL;
    PyObject *t = NULL;
    PyObject *key, *exponent, *quantized;
    _Py_IDENTIFIER(quantize);
    _Py_IDENTIFIER(__truediv__);

    if (!module) {
        module = PyImport_ImportModuleNoBlock("decimal");
        if (module == NULL)
            return NULL;
    }

    if (!decimal) {
        decimal = PyObject_GetAttrString(module, "Decimal");
        if (decimal == NULL)
            return NULL;
    }

    if (context == NULL)
    {
        /* Use 12 decimal digits to store 10,000 years in seconds + 9
           decimal digits for the floating part in nanoseconds + 1 decimal
           digit to round correctly.

           context = decimal.Context(22, rounding=decimal.ROUND_HALF_EVEN)
           exponent_context = decimal.Context(1, rounding=decimal.ROUND_HALF_EVEN)
        */
        PyObject *context_class, *rounding;
        context_class = PyObject_GetAttrString(module, "Context");
        if (context_class == NULL)
            return NULL;
        rounding = PyObject_GetAttrString(module, "ROUND_HALF_EVEN");
        if (rounding == NULL)
        {
            Py_DECREF(context_class);
            return NULL;
        }
        context = PyObject_CallFunction(context_class, "iO", 22, rounding);
        if (context == NULL)
        {
            Py_DECREF(context_class);
            Py_DECREF(rounding);
            return NULL;
        }

        exponent_context = PyObject_CallFunction(context_class, "iO", 1, rounding);
        Py_DECREF(context_class);
        Py_DECREF(rounding);
        if (exponent_context == NULL)
        {
            Py_CLEAR(context);
            return NULL;
        }
    }

    /* t = decimal.Decimal(value) */
    if (ts->seconds) {
        PyObject *f = _PyLong_FromTime_t(ts->seconds);
        t = PyObject_CallFunction(decimal, "O", f);
        Py_CLEAR(f);
    }
    else {
        t = PyObject_CallFunction(decimal, "iO", 0, context);
    }
    if (t == NULL)
        return NULL;

    if (ts->numerator)
    {
        /* t += decimal.Decimal(numerator, ctx) / decimal.Decimal(denominator, ctx) */
        PyObject *a, *b, *c, *d, *x;

        x = _PyLong_FromTimeFraction_t(ts->numerator);
        if (x == NULL)
            goto error;
        a = PyObject_CallFunction(decimal, "OO", x, context);
        Py_CLEAR(x);
        if (a == NULL)
            goto error;

        x = _PyLong_FromTimeFraction_t(ts->denominator);
        if (x == NULL)
        {
            Py_DECREF(a);
            goto error;
        }
        b = PyObject_CallFunction(decimal, "OO", x, context);
        Py_CLEAR(x);
        if (b == NULL)
        {
            Py_DECREF(a);
            goto error;
        }

        c = _PyObject_CallMethodId(a, &PyId___truediv__, "OO",
                                   b, context);
        Py_DECREF(a);
        Py_DECREF(b);
        if (c == NULL)
            goto error;

        d = PyNumber_Add(t, c);
        Py_DECREF(c);
        if (d == NULL)
            goto error;
        Py_DECREF(t);
        t = d;
    }

    if (exponent_cache == NULL) {
        exponent_cache = PyDict_New();
        if (exponent_cache == NULL)
            goto error;
    }

    key = _PyLong_FromTimeFraction_t(ts->denominator);
    if (key == NULL)
        goto error;
    exponent = PyDict_GetItem(exponent_cache, key);
    if (exponent == NULL) {
        /* exponent = decimal.Decimal(1) / decimal.Decimal(resolution) */
        PyObject *one, *denominator;

        one = PyObject_CallFunction(decimal, "i", 1);
        if (one == NULL) {
            Py_DECREF(key);
            goto error;
        }

        denominator = PyObject_CallFunction(decimal, "O", key);
        if (denominator == NULL) {
            Py_DECREF(key);
            Py_DECREF(one);
            goto error;
        }

        exponent = _PyObject_CallMethodId(one, &PyId___truediv__, "OO",
                                          denominator, exponent_context);
        Py_DECREF(one);
        Py_DECREF(denominator);
        if (exponent == NULL) {
            Py_DECREF(key);
            goto error;
        }

        if (PyDict_SetItem(exponent_cache, key, exponent) < 0) {
            Py_DECREF(key);
            Py_DECREF(exponent);
            goto error;
        }
        Py_DECREF(key);
    }

    /* t = t.quantize(exponent, None, context) */
    quantized = _PyObject_CallMethodId(t, &PyId_quantize, "OOO",
                                       exponent, Py_None, context);
    if (quantized == NULL)
        goto error;
    Py_DECREF(t);
    t = quantized;

    return t;

error:
    Py_XDECREF(t);
    return NULL;
}

PyObject*
_PyTime_Convert(_PyTime_t *ts, PyObject *format)
{
    assert(ts->denominator != 0);

    if (format == NULL || (PyTypeObject *)format == &PyFloat_Type)
        return _PyTime_AsFloat(ts);
    if ((PyTypeObject *)format == &PyLong_Type)
        return _PyTime_AsLong(ts);

    if (PyType_Check(format))
    {
        PyObject *module, *name;
        _Py_IDENTIFIER(__name__);
        _Py_IDENTIFIER(__module__);

        module = _PyObject_GetAttrId(format, &PyId___module__);
        name = _PyObject_GetAttrId(format, &PyId___name__);
        if (module != NULL && PyUnicode_Check(module)
            && name != NULL && PyUnicode_Check(name))
        {
            if (PyUnicode_CompareWithASCIIString(module, "decimal") == 0
                && PyUnicode_CompareWithASCIIString(name, "Decimal") == 0)
                return _PyTime_AsDecimal(ts);
        }
        else
            PyErr_Clear();
    }

    PyErr_Format(PyExc_ValueError, "Unknown timestamp format: %R", format);
    return NULL;
}

void
_PyTime_Init()
{
    /* Do nothing.  Needed to force linking. */
}
